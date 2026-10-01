#include "CapeNvCloth.h"
#include <NvCloth/Factory.h>
#include <NvCloth/Cloth.h>
#include <NvCloth/Fabric.h>
#include <NvCloth/Solver.h>
#include <NvCloth/PhaseConfig.h>
#include <NvClothExt/ClothFabricCooker.h>
#include <foundation/PxErrorCallback.h>
#include <foundation/PxQuat.h>
#include <cstdlib>
#include <mutex>

namespace cape {
namespace {
using physx::PxVec3;using physx::PxVec4;
PxVec3 nv(Vec3 p){return {p.x,p.y,p.z};}
Vec3 vec(const PxVec4& p){return {p.x,p.y,p.z};}
struct Allocator final:physx::PxAllocatorCallback {
    void* allocate(size_t n,const char*,const char*,int) override {
        void* raw=std::malloc(n+16+sizeof(void*));if(!raw)std::abort();
        auto at=(reinterpret_cast<std::uintptr_t>(raw)+sizeof(void*)+15)&~std::uintptr_t(15);
        auto* aligned=reinterpret_cast<void*>(at);reinterpret_cast<void**>(aligned)[-1]=raw;return aligned;
    }
    void deallocate(void* p) override {if(p)std::free(reinterpret_cast<void**>(p)[-1]);}
};
thread_local bool failed=false;
struct Errors final:physx::PxErrorCallback {
    void reportError(physx::PxErrorCode::Enum code,const char*,const char*,int) override {
        if(code!=physx::PxErrorCode::eDEBUG_INFO&&code!=physx::PxErrorCode::eDEBUG_WARNING)failed=true;
    }
};
struct Asserts final:nv::cloth::PxAssertHandler {
    void operator()(const char*,const char*,int,bool&) override {failed=true;}
};
void initializeLibrary(){
    // Callback lifetime exceeds every solver, including worker teardown.
    static std::once_flag once;
    std::call_once(once,[]{nv::cloth::InitializeNvCloth(new Allocator,new Errors,new Asserts,nullptr);});
}
bool validPose(const std::vector<Vec3>& pose,std::size_t count){
    if(pose.size()!=count)return false;
    for(auto p:pose)if(!finite(p)||std::fabs(p.x)>1.e6f||std::fabs(p.y)>1.e6f||std::fabs(p.z)>1.e6f)return false;
    return true;
}
}
struct NvClothSolver::Impl {
    nv::cloth::Factory* factory=nullptr;nv::cloth::Fabric* fabric=nullptr;
    nv::cloth::Cloth* cloth=nullptr;nv::cloth::Solver* solver=nullptr;
    bool added=false;Vec3 origin{};double time=0;
    std::vector<std::uint32_t> pins;std::vector<float> masses;
    std::vector<Vec3> lastPose;
    ~Impl(){
        if(added)solver->removeCloth(cloth);
        if(cloth)NV_CLOTH_DELETE(cloth);
        if(fabric)fabric->decRefCount();
        if(solver)NV_CLOTH_DELETE(solver);
        if(factory)NvClothDestroyFactory(factory);
    }
};
NvClothSolver::NvClothSolver()=default;
NvClothSolver::~NvClothSolver()=default;
NvClothSolver::NvClothSolver(NvClothSolver&&) noexcept=default;
NvClothSolver& NvClothSolver::operator=(NvClothSolver&&) noexcept=default;
bool NvClothSolver::initialize(const std::vector<Vec3>& pose,const std::vector<Triangle>& faces,const std::vector<std::uint32_t>& pins,Config config){
    impl_.reset();positions_.clear();stats_={};config_=config;
    if(pose.size()<3||pose.size()>config.maxVertices||faces.empty()||faces.size()>config.maxTriangles||pins.empty()||!validPose(pose,pose.size()))return false;
    if(!std::isfinite(config.fixedStep)||config.fixedStep<.001f||config.fixedStep>1.f/30||config.maxSubsteps==0||config.maxSubsteps>32||!finite(config.gravity)||!std::isfinite(config.poseLimit)||config.poseLimit<0||!std::isfinite(config.damping)||config.damping<0||!std::isfinite(config.density)||config.density<=0||!std::isfinite(config.maxSpeed)||config.maxSpeed<=0)return false;
    if(!std::isfinite(config.clothBending)||config.clothBending<0||config.clothBending>1||!std::isfinite(config.clothAir)||config.clothAir<0||config.clothAir>1)return false;
    for(auto pin:pins)if(pin>=pose.size())return false;
    for(auto t:faces)if(t.a>=pose.size()||t.b>=pose.size()||t.c>=pose.size()||length(cross(pose[t.b]-pose[t.a],pose[t.c]-pose[t.a]))<1.e-9f)return false;
    initializeLibrary();failed=false;auto state=std::make_unique<Impl>();
    state->pins=pins;state->masses.assign(pose.size(),0);
    // NvCloth's aerodynamic forces use inverse mass. One kilogram per vertex
    // made a small cape weigh tens of kilograms and barely respond to air.
    for(auto face:faces){const float mass=length(cross(pose[face.b]-pose[face.a],pose[face.c]-pose[face.a]))*.5f*config.density/3;
        for(auto id:{face.a,face.b,face.c})state->masses[id]+=mass;}
    for(auto& mass:state->masses)mass=1.f/std::max(mass,.002f);
    for(auto pin:pins)state->masses[pin]=0;
    material_=pose;
    state->origin=pose[pins[0]];state->lastPose=pose;
    std::vector<PxVec3> points;std::vector<PxVec4> particles;
    for(unsigned i=0;i<pose.size();++i){points.push_back(nv(pose[i]-state->origin));particles.emplace_back(points.back(),state->masses[i]);}
    state->factory=NvClothCreateFactoryCPU();if(!state->factory)return false;
    nv::cloth::ClothMeshDesc desc;
    desc.points.data=points.data();desc.points.count=static_cast<unsigned>(points.size());desc.points.stride=sizeof(PxVec3);
    desc.invMasses.data=state->masses.data();desc.invMasses.count=static_cast<unsigned>(pose.size());desc.invMasses.stride=sizeof(float);
    desc.triangles.data=faces.data();desc.triangles.count=static_cast<unsigned>(faces.size());desc.triangles.stride=sizeof(Triangle);
    nv::cloth::Vector<int32_t>::Type types;
    state->fabric=NvClothCookFabricFromMesh(state->factory,desc,nv(normalized(config.gravity,{0,0,-1})),&types,true);
    if(!state->fabric||failed)return false;
    state->cloth=state->factory->createCloth({particles.data(),particles.data()+particles.size()},*state->fabric);
    if(!state->cloth||failed)return false;
    auto& cloth=*state->cloth;
    std::vector<nv::cloth::PhaseConfig> phases;
    for(unsigned i=0;i<types.size();++i){nv::cloth::PhaseConfig phase(static_cast<std::uint16_t>(i));
        phase.mStiffness=types[i]==nv::cloth::ClothFabricPhaseType::eBENDING?config.clothBending:1.f;phases.push_back(phase);
    }
    cloth.setPhaseConfig({phases.data(),phases.data()+phases.size()});
    cloth.teleportToLocation(nv(state->origin),physx::PxQuat(physx::PxIdentity));
    cloth.setGravity(nv(config.gravity));cloth.setSolverFrequency(240);cloth.setStiffnessFrequency(60);
    cloth.setDamping(PxVec3(1.f-std::exp(-config.damping/60.f)));
    cloth.setLinearDrag(PxVec3(0));cloth.setAngularDrag(PxVec3(0));
    cloth.setTetherConstraintStiffness(1);cloth.setTetherConstraintScale(1);
    cloth.setFriction(.25f);cloth.setMotionConstraintStiffness(1);cloth.setSleepThreshold(0);
    // Air at rest in world space produces relative drag during movement.
    // Keep lift disabled; it caused launches in the previous contact solver.
    cloth.setDragCoefficient(config.clothAir);cloth.setLiftCoefficient(0);
    cloth.setFluidDensity(1.225f);cloth.setWindVelocity(PxVec3(0));
    state->solver=state->factory->createSolver();if(!state->solver)return false;
    state->solver->addCloth(state->cloth);state->added=true;
    if(failed)return false;
    impl_=std::move(state);positions_=pose;return true;
}
void NvClothSolver::reset(const std::vector<Vec3>& pose){
    if(!impl_||!validPose(pose,positions_.size()))return;
    auto& s=*impl_;s.origin=pose[s.pins[0]];s.time=0;s.lastPose=pose;positions_=pose;
    auto current=s.cloth->getCurrentParticles();auto previous=s.cloth->getPreviousParticles();
    for(unsigned i=0;i<pose.size();++i)current[i]=previous[i]=PxVec4(nv(pose[i]-s.origin),s.masses[i]);
    s.cloth->teleportToLocation(nv(s.origin),physx::PxQuat(physx::PxIdentity));
    s.cloth->clearMotionConstraints();s.cloth->clearSeparationConstraints();s.cloth->clearInterpolation();
    stats_.reset=true;
}
bool NvClothSolver::step(float elapsed,const std::vector<Vec3>& pose,const std::vector<ColliderTriangle>& surfaces,const std::vector<ColliderBox>& boxes,const std::vector<Vec3>&){
    stats_={};failed=false;
    if(!impl_||!validPose(pose,positions_.size())||!std::isfinite(elapsed)||elapsed<0){stats_.invalidInput=true;return false;}
    if(surfaces.size()>config_.maxColliderTriangles||boxes.size()>config_.maxColliderBoxes){stats_.budgetExceeded=true;return false;}
    for(const auto& box:boxes){if(!finite(box.currentCenter)||!finite(box.currentHalf)||!finite(box.preferredDirection)){stats_.invalidInput=true;return false;}for(auto axis:box.currentAxes)if(!finite(axis)){stats_.invalidInput=true;return false;}}
    for(const auto& surface:surfaces)for(auto p:surface.current)if(!finite(p)){stats_.invalidInput=true;return false;}
    auto& s=*impl_;bool teleport=elapsed>config_.maxFrameTime;
    for(auto pin:s.pins)if(length(pose[pin]-s.lastPose[pin])>config_.teleportDistance)teleport=true;
    if(teleport){reset(pose);elapsed=0;}
    // Use NvCloth's moving frame rather than manually rebasing world-space
    // velocity. Damping and the speed guard must act on motion relative to
    // the player, otherwise they brake the entire running character.
    const Vec3 origin=pose[s.pins[0]];s.origin=origin;
    {
        auto current=s.cloth->getCurrentParticles();auto previous=s.cloth->getPreviousParticles();
        for(unsigned i=0;i<pose.size();++i){
            if(s.masses[i]==0)current[i]=previous[i]=PxVec4(nv(pose[i]-origin),0);
        }
    }
    // Free cloth has no targets sampled from the native cape animation.
    // Only inverse-mass-zero shoulder/collar particles follow that pose.
    s.cloth->clearMotionConstraints();s.cloth->clearSeparationConstraints();
    // Query body contacts in the current attachment frame, not at last frame's
    // world location while the player is running.
    std::vector<Vec3> contactPose;contactPose.reserve(positions_.size());
    {auto particles=s.cloth->getCurrentParticles();for(unsigned i=0;i<positions_.size();++i)contactPose.push_back(vec(particles[i])+origin);}
    const auto contacts=skinCapeContacts(colliders_,contactPose);
    auto separation=s.cloth->getSeparationConstraints();
    for(unsigned i=0;i<positions_.size();++i){
        const auto hit=s.masses[i]>0?capeMeshContact(contactPose[i],contacts):CapeContact{};
        separation[i]=PxVec4(nv(hit.center-origin),hit.radius);if(hit.radius>0)++stats_.contacts;
    }
    std::vector<PxVec3> triangles;triangles.reserve(surfaces.size()*3);
    for(const auto& surface:surfaces)for(auto p:surface.current)triangles.push_back(nv(p-origin));
    s.cloth->setTriangles({triangles.data(),triangles.data()+triangles.size()},0,s.cloth->getNumTriangles());
    // Constraints are supplied in the current moving frame. Their previous
    // centers must not refer to a different captured skeletal pose.
    s.cloth->clearInterpolation();
    s.time+=elapsed;unsigned steps=static_cast<unsigned>((s.time+1.e-9)/config_.fixedStep);
    if(steps>config_.maxSubsteps){steps=config_.maxSubsteps;s.time=steps*config_.fixedStep;stats_.budgetExceeded=true;}
    const auto start=s.cloth->getTranslation();
    for(unsigned step=0;step<steps;++step){
        s.cloth->setTranslation(start+(nv(origin)-start)*(float(step+1)/steps));
        if(!s.solver->beginSimulation(config_.fixedStep))return false;
        const auto chunks=s.solver->getSimulationChunkCount();
        for(int chunk=0;chunk<chunks;++chunk)s.solver->simulateChunk(chunk);
        s.solver->endSimulation();++stats_.substeps;
    }
    s.time-=steps*config_.fixedStep;s.lastPose=pose;
    auto current=s.cloth->getCurrentParticles();auto previous=s.cloth->getPreviousParticles();
    for(unsigned i=0;i<pose.size();++i){
        if(s.masses[i]==0)current[i]=previous[i]=PxVec4(nv(pose[i]-origin),0);
        positions_[i]=vec(current[i])+origin;
        auto velocity=vec(current[i])-vec(previous[i]);const float speed=length(velocity),maxDelta=config_.maxSpeed*std::max(s.cloth->getPreviousIterationDt(),1.e-5f);
        if(speed>maxDelta)previous[i]=PxVec4(nv(vec(current[i])-velocity*(maxDelta/speed)),s.masses[i]);
    }
    return !failed&&validPose(positions_,pose.size());
}
void NvClothSolver::acceptFit(const std::vector<Vec3>& pose,float fraction){
    auto current=impl_->cloth->getCurrentParticles();auto previous=impl_->cloth->getPreviousParticles();
    for(unsigned i=0;i<pose.size();++i){
        const auto velocity=(vec(current[i])-vec(previous[i]))*fraction;
        positions_[i]=lerp(pose[i],positions_[i],fraction);
        current[i]=PxVec4(nv(positions_[i]-impl_->origin),impl_->masses[i]);
        previous[i]=PxVec4(nv(vec(current[i])-velocity),impl_->masses[i]);
    }
}
}
