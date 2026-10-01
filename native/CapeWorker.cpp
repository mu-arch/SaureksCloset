#include "CapeWorker.h"
#include "CapeFit.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#ifdef _WIN32
#include <windows.h>
#endif
namespace cape {
namespace {
struct Job {
    std::uint64_t generation=0,serial=0;double time=0;Config config;
    Rotation frame=identityRotation();std::chrono::steady_clock::time_point sampled;
    std::vector<Vec3> pose;std::vector<Triangle> faces;std::vector<std::uint32_t> pins;
    std::vector<ColliderTriangle> surfaces;std::vector<ColliderBox> boxes;std::vector<AnimatedCollider> colliders;
};
struct Result {
    std::uint64_t generation=0,serial=0;bool okay=false;Stats stats;
    Rotation frame=identityRotation();std::vector<Vec3> offsets;
    std::chrono::steady_clock::time_point finished;
};
class Worker {
    std::mutex mutex_;std::condition_variable condition_;
    Job pending_;Result result_;bool pendingValid_=false,stopping_=false,running_=false;
    std::thread thread_;
    std::atomic<std::uint64_t> wanted_{0};std::uint64_t serial_=0;
#ifndef _WIN32
    std::atomic<unsigned> delay_{0};
#endif
    void run(){
        NvClothSolver solver;std::uint64_t generation=0;double previousTime=0;
        for(;;){
            Job job;
            {std::unique_lock<std::mutex> lock(mutex_);condition_.wait(lock,[&]{return stopping_||pendingValid_;});
                if(stopping_)return;
                job=std::move(pending_);pendingValid_=false;running_=true;
            }
#ifndef _WIN32
            const auto delay=delay_.load();if(delay)std::this_thread::sleep_for(std::chrono::milliseconds(delay));
#endif
            bool okay=true;
            if(generation!=job.generation){okay=solver.initialize(job.pose,job.faces,job.pins,job.config);generation=job.generation;previousTime=job.time;}
            solver.setColliders(job.colliders);
            if(okay)okay=solver.step(static_cast<float>(std::max(0.,job.time-previousTime)),job.pose,job.surfaces,job.boxes,job.pose);
            previousTime=job.time;
            if(okay)okay=capeFabricFits(solver.positions(),job.pose,solver.material(),job.faces,job.pins);
            Result out;out.generation=job.generation;out.serial=job.serial;out.okay=okay;out.stats=solver.stats();out.frame=job.frame;
            if(okay){out.offsets.resize(job.pose.size());for(unsigned i=0;i<job.pose.size();++i)out.offsets[i]=solver.positions()[i]-job.pose[job.pins[0]];}
            out.finished=job.sampled;
            {std::lock_guard<std::mutex> lock(mutex_);running_=false;
                if(wanted_.load()==job.generation)result_=std::move(out);
            }
            condition_.notify_all();
        }
    }
public:
    Worker(){
#ifdef _WIN32
        // No unloading code while its persistent worker is executing. Called
        // lazily by the first cape draw, never from DllMain/loader lock.
        HMODULE module=nullptr;GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCSTR>(&workerModuleAddress),&module);
#endif
        thread_=std::thread([this]{run();});
    }
#ifdef _WIN32
    static void workerModuleAddress(){}
#endif
    ~Worker(){
        {std::lock_guard<std::mutex> lock(mutex_);stopping_=true;}
        condition_.notify_one();if(thread_.joinable())thread_.join();
    }
    void want(std::uint64_t generation){wanted_.store(generation);}
    bool exchange(Job&& job,Result& result){
        // Render never waits for the worker, even while it publishes a result.
        std::unique_lock<std::mutex> lock(mutex_,std::try_to_lock);if(!lock.owns_lock())return false;
        job.serial=++serial_;pending_=std::move(job);pendingValid_=true;
        if(result_.generation==pending_.generation)result=result_;
        lock.unlock();condition_.notify_one();return true;
    }
#ifndef _WIN32
    void wait(){std::unique_lock<std::mutex> lock(mutex_);condition_.wait(lock,[&]{return !pendingValid_&&!running_;});}
    void delay(unsigned milliseconds){delay_.store(milliseconds);}
#endif
};
Worker& worker(){
#ifdef _WIN32
    // Process-lifetime pinned module: Windows terminates this thread on exit.
    // Do not join from a DLL static destructor under the loader lock.
    static Worker* instance=new Worker;return *instance;
#else
    static Worker instance;return instance;
#endif
}
std::uint64_t nextGeneration(){static std::atomic<std::uint64_t> next{0};return ++next;}
Vec3 rotateDelta(Vec3 value,const Rotation& from,const Rotation& to){
    return to[0]*dot(value,from[0])+to[1]*dot(value,from[1])+to[2]*dot(value,from[2]);
}
}
bool AsyncCloth::initialize(const std::vector<Vec3>& pose,const std::vector<Triangle>& faces,const std::vector<std::uint32_t>& pins,Config config){
    if(pose.size()<3||pose.size()>config.maxVertices||faces.empty()||faces.size()>config.maxTriangles||pins.empty())return false;
    for(auto p:pose)if(!finite(p))return false;
    for(auto pin:pins)if(pin>=pose.size())return false;
    for(auto t:faces)if(t.a>=pose.size()||t.b>=pose.size()||t.c>=pose.size())return false;
    config_=config;material_=pose;faces_=faces;pins_=pins;initialized_=true;reset(pose);return true;
}
void AsyncCloth::reset(const std::vector<Vec3>& pose){
    if(!initialized_)return;
    generation_=nextGeneration();worker().want(generation_);time_=0;received_=0;hasResult_=false;stats_={};
    positions_=pose;resultOffsets_.clear();
}
bool AsyncCloth::step(float elapsed,const std::vector<Vec3>& pose,const std::vector<ColliderTriangle>& surfaces,const std::vector<ColliderBox>& boxes,const std::vector<Vec3>&){
    if(!initialized_||pose.size()!=positions_.size()||!std::isfinite(elapsed)||elapsed<0)return false;
    for(auto p:pose)if(!finite(p))return false;
    if(surfaces.size()>config_.maxColliderTriangles||boxes.size()>config_.maxColliderBoxes)return false;
    bool teleport=elapsed>config_.maxFrameTime;
    for(auto pin:pins_)if(length(pose[pin]-positions_[pin])>config_.teleportDistance)teleport=true;
    if(teleport)reset(pose);
    time_+=std::min(elapsed,config_.maxFrameTime);
    Job job;job.generation=generation_;job.time=time_;job.config=config_;job.frame=frame_;job.sampled=std::chrono::steady_clock::now();
    job.pose=pose;job.faces=faces_;job.pins=pins_;job.surfaces=surfaces;job.boxes=boxes;job.colliders=colliders_;
    Result result;worker().exchange(std::move(job),result);
    if(result.generation==generation_&&result.serial>received_){
        received_=result.serial;stats_=result.stats;
        if(!result.okay){hasResult_=false;resultOffsets_.clear();return false;}
        // A suspended/overloaded worker may not reintroduce an old pose.
        if(std::chrono::steady_clock::now()-result.finished<std::chrono::milliseconds(250)){
            resultOffsets_=std::move(result.offsets);resultFrame_=result.frame;resultTime_=result.finished;hasResult_=true;
        }
    }
    if(hasResult_&&std::chrono::steady_clock::now()-resultTime_>=std::chrono::milliseconds(250)){hasResult_=false;resultOffsets_.clear();}
    positions_=pose;
    if(hasResult_&&resultOffsets_.size()==pose.size())for(unsigned i=0;i<pose.size();++i)positions_[i]=pose[pins_[0]]+rotateDelta(resultOffsets_[i],resultFrame_,frame_);
    for(auto pin:pins_)positions_[pin]=pose[pin];
    return true;
}
#ifndef _WIN32
void waitCapeWorkerForTests(){worker().wait();}
void delayCapeWorkerForTests(unsigned milliseconds){worker().delay(milliseconds);}
#endif
}
