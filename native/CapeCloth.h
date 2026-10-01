#pragma once
// Bounded CPU cloth for the player's original cape triangles. Coordinates and
// gravity are in WORLD space; only the explicitly supplied seam is kinematic.
// No skeleton, cape dimensions, procedural waveform, or replacement mesh lives
// here. Animated positions are used for pins and safe resets, never as springs
// pulling free cloth back to an animation.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

namespace cape {
struct Vec3 { float x=0, y=0, z=0; };
inline Vec3 operator+(Vec3 a,Vec3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline Vec3 operator-(Vec3 a,Vec3 b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline Vec3 operator-(Vec3 a){return {-a.x,-a.y,-a.z};}
inline Vec3 operator*(Vec3 a,float s){return {a.x*s,a.y*s,a.z*s};}
inline Vec3 operator/(Vec3 a,float s){return a*(1.f/s);}
inline Vec3& operator+=(Vec3& a,Vec3 b){a=a+b;return a;}
inline Vec3& operator-=(Vec3& a,Vec3 b){a=a-b;return a;}
inline float dot(Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline Vec3 cross(Vec3 a,Vec3 b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline float length(Vec3 a){return std::sqrt(dot(a,a));}
inline bool finite(Vec3 a){return std::isfinite(a.x)&&std::isfinite(a.y)&&std::isfinite(a.z);}
inline Vec3 normalized(Vec3 a,Vec3 fallback={0,0,1}){const float n=length(a);return n>1.e-10f&&std::isfinite(n)?a/n:fallback;}
inline Vec3 lerp(Vec3 a,Vec3 b,float t){return a+(b-a)*t;}
inline float clamp(float x,float a,float b){return std::max(a,std::min(b,x));}
struct Triangle { std::uint32_t a=0,b=0,c=0; };
struct ColliderTriangle {
    Vec3 previous[3]{},current[3]{};
};
struct Config {
    Vec3 gravity{0,0,-9.81f};
    float fixedStep=1.f/120.f;
    unsigned maxSubsteps=12, iterations=8;
    float thickness=.015f, selfThickness=.02f;
    // Extra collision samples cover sparse original cape edges and faces.
    // Samples apply barycentric corrections to existing mesh vertices only.
    float contactSpacing=.06f;
    unsigned maxContactSamples=2048;
    float stretchCompliance=1.e-7f, areaCompliance=1.e-8f;
    // Opposite vertices of neighboring triangles form a compliant bending
    // spring. This is a bending approximation, not a dihedral-angle solver.
    float bendCompliance=.03f;
    float density=.25f, damping=2.f, friction=.12f;
    float maxSpeed=40.f, teleportDistance=3.f, maxFrameTime=.25f;
    bool selfCollision=true;
    unsigned maxVertices=2048, maxTriangles=4096, maxColliderTriangles=8192;
    unsigned maxCollisionTests=400000, maxSelfPairs=100000;
};
struct Stats {
    unsigned substeps=0, collisionTests=0, contacts=0, selfPairs=0;
    bool reset=false, invalidInput=false, budgetExceeded=false;
};

class Cloth {
    struct Spring {std::uint32_t a,b;float rest,compliance,lambda=0;};
    struct Face {Triangle tri;float restArea,lambda=0;};
    struct Edge {std::uint32_t a,b,opposite;};
    struct Box {Vec3 lo{1.e30f,1.e30f,1.e30f},hi{-1.e30f,-1.e30f,-1.e30f};};
    struct Node {Box box;unsigned begin=0,count=0,left=0,right=0;};
    struct Closest {Vec3 point;float u=1,v=0,w=0;};
    struct Cell {int x,y,z;std::uint32_t vertex;};
    struct Sample {std::uint32_t ids[3];float weights[3];};
    Config config_{};
    Stats stats_{};
    bool ready_=false;
    double accumulator_=0;
    std::vector<Vec3> x_,velocity_,old_,lastPose_,contactNormal_,contactVelocity_;
    std::vector<float> invMass_;
    std::vector<std::uint32_t> pins_;
    std::vector<Spring> stretch_,bend_;
    std::vector<Face> faces_;
    std::vector<std::uint64_t> adjacent_;
    std::vector<ColliderTriangle> colliders_;
    std::vector<unsigned> order_;
    std::vector<Node> nodes_;
    std::vector<Cell> cells_;
    std::vector<Sample> samples_;
    static std::uint64_t pair(std::uint32_t a,std::uint32_t b){if(a>b)std::swap(a,b);return (std::uint64_t(a)<<32)|b;}
    static void grow(Box& b,Vec3 p){b.lo={std::min(b.lo.x,p.x),std::min(b.lo.y,p.y),std::min(b.lo.z,p.z)};b.hi={std::max(b.hi.x,p.x),std::max(b.hi.y,p.y),std::max(b.hi.z,p.z)};}
    static void grow(Box& b,const Box& a){grow(b,a.lo);grow(b,a.hi);}
    static bool overlaps(const Box& a,const Box& b){return a.lo.x<=b.hi.x&&a.hi.x>=b.lo.x&&a.lo.y<=b.hi.y&&a.hi.y>=b.lo.y&&a.lo.z<=b.hi.z&&a.hi.z>=b.lo.z;}
    static Box triangleBox(const ColliderTriangle& t){Box b;for(unsigned i=0;i<3;++i){grow(b,t.previous[i]);grow(b,t.current[i]);}return b;}
    static float component(Vec3 v,unsigned axis){return axis==0?v.x:(axis==1?v.y:v.z);}
    unsigned buildNode(unsigned begin,unsigned end){
        const unsigned index=static_cast<unsigned>(nodes_.size());nodes_.push_back({});
        Box bounds;for(unsigned i=begin;i<end;++i)grow(bounds,triangleBox(colliders_[order_[i]]));
        nodes_[index].box=bounds;
        if(end-begin<=8){nodes_[index].begin=begin;nodes_[index].count=end-begin;return index;}
        const Vec3 span=bounds.hi-bounds.lo;const unsigned axis=span.x>span.y?(span.x>span.z?0:2):(span.y>span.z?1:2);
        const unsigned middle=(begin+end)/2;
        std::nth_element(order_.begin()+begin,order_.begin()+middle,order_.begin()+end,[&](unsigned a,unsigned b){
            const auto aa=triangleBox(colliders_[a]),bb=triangleBox(colliders_[b]);
            return component(aa.lo+aa.hi,axis)<component(bb.lo+bb.hi,axis);
        });
        const unsigned left=buildNode(begin,middle),right=buildNode(middle,end);
        nodes_[index].left=left;nodes_[index].right=right;return index;
    }
    static Closest closest(Vec3 p,Vec3 a,Vec3 b,Vec3 c){
        // Voronoi-region closest point, including finite edges and corners.
        if(dot(cross(b-a,c-a),cross(b-a,c-a))<1.e-20f){
            // A collider can become degenerate during animation. Its edges
            // still collide; no division by a vanishing triangle area.
            Closest best{a,1,0,0};float distance=dot(p-a,p-a);
            const Vec3 edgeA[3]={a,b,c},edgeB[3]={b,c,a};
            for(unsigned i=0;i<3;++i){const Vec3 d=edgeB[i]-edgeA[i];const float dd=dot(d,d);const float t=dd>1.e-20f?clamp(dot(p-edgeA[i],d)/dd,0,1):0;const Vec3 q=edgeA[i]+d*t;const float qq=dot(p-q,p-q);if(qq<distance){distance=qq;best={q,i==0?1-t:(i==2?t:0),i==0?t:(i==1?1-t:0),i==1?t:(i==2?1-t:0)};}}
            return best;
        }
        const Vec3 ab=b-a,ac=c-a,ap=p-a;
        const float d1=dot(ab,ap),d2=dot(ac,ap);
        if(d1<=0&&d2<=0)return {a,1,0,0};
        const Vec3 bp=p-b;const float d3=dot(ab,bp),d4=dot(ac,bp);
        if(d3>=0&&d4<=d3)return {b,0,1,0};
        const float vc=d1*d4-d3*d2;
        if(vc<=0&&d1>=0&&d3<=0){const float v=d1/(d1-d3);return {a+ab*v,1-v,v,0};}
        const Vec3 cp=p-c;const float d5=dot(ab,cp),d6=dot(ac,cp);
        if(d6>=0&&d5<=d6)return {c,0,0,1};
        const float vb=d5*d2-d1*d6;
        if(vb<=0&&d2>=0&&d6<=0){const float w=d2/(d2-d6);return {a+ac*w,1-w,0,w};}
        const float va=d3*d6-d5*d4;
        if(va<=0&&(d4-d3)>=0&&(d5-d6)>=0){const float w=(d4-d3)/((d4-d3)+(d5-d6));return {b+(c-b)*w,0,1-w,w};}
        const float denom=va+vb+vc;

        const float v=vb/denom,w=vc/denom;return {a+ab*v+ac*w,1-v-w,v,w};
    }
    static void sample(const ColliderTriangle& c,float t,Vec3 (&v)[3]){for(unsigned i=0;i<3;++i)v[i]=lerp(c.previous[i],c.current[i],t);}
    static Vec3 carryNormal(Vec3 normal,Vec3 from,Vec3 to){
        from=normalized(from);to=normalized(to,from);
        const Vec3 axis=cross(from,to);const float cosine=clamp(dot(from,to),-1,1);
        // Rodrigues' minimal rotation; near a 180-degree flip the caller's
        // fixed substeps and animation/teleport resets bound the ambiguity.
        if(cosine<-.999f)return -normal;
        return normalized(normal+cross(axis,normal)+cross(axis,cross(axis,normal))/(1+cosine),normal);
    }
    bool contact(Vec3 start,Vec3 finish,const ColliderTriangle& c,float from,float to,float h,Vec3& correction,Vec3& contactNormal,Vec3& contactVelocity){
        Vec3 a[3],b[3];sample(c,from,a);sample(c,to,b);
        const Vec3 travel=finish-start;
        const float radius=config_.thickness,tolerance=std::max(1.e-6f,radius*.002f);
        float speed=0;for(unsigned i=0;i<3;++i)speed=std::max(speed,length(travel-(b[i]-a[i])));
        float t=0;Closest hit{};Vec3 hitVertices[3]{},normal{};bool found=false;
        // Point/triangle distance is Lipschitz with this relative speed bound.
        // Advancement cannot skip a thin or moving triangle. If the iteration
        // cap is reached we retain a conservative contact, rather than taking
        // an unverified leap through the surface.
        for(unsigned iteration=0;iteration<32;++iteration){
            for(unsigned i=0;i<3;++i)hitVertices[i]=lerp(a[i],b[i],t);
            const Vec3 p=lerp(start,finish,t);
            hit=closest(p,hitVertices[0],hitVertices[1],hitVertices[2]);
            const float distance=length(p-hit.point);
            if(distance<=radius+tolerance||iteration==31){
                normal=normalized(p-hit.point,normalized(cross(hitVertices[1]-hitVertices[0],hitVertices[2]-hitVertices[0])));
                if(distance<1.e-8f){const Vec3 relative=travel-((b[0]-a[0])*hit.u+(b[1]-a[1])*hit.v+(b[2]-a[2])*hit.w);if(dot(normal,relative)>0)normal=-normal;}
                found=true;break;
            }
            if(speed<1.e-10f)return false;
            const float advance=(distance-radius)/speed;
            if(t+advance>1.f)return false;
            t+=advance*.95f;
        }
        if(!found)return false;
        const Vec3 finalNormal=carryNormal(normal,cross(hitVertices[1]-hitVertices[0],hitVertices[2]-hitVertices[0]),cross(b[1]-b[0],b[2]-b[0]));
        const Vec3 finalPoint=b[0]*hit.u+b[1]*hit.v+b[2]*hit.w;
        const float separation=dot(finish-finalPoint,finalNormal);
        if(separation>=radius)return false;
        correction=finalNormal*(radius-separation);contactNormal=finalNormal;
        contactVelocity=((b[0]-a[0])*hit.u+(b[1]-a[1])*hit.v+(b[2]-a[2])*hit.w)/h;
        ++stats_.contacts;return true;
    }
    bool collisions(float from,float to,float h){
        if(nodes_.empty())return true;
        const Vec3 padding{config_.thickness,config_.thickness,config_.thickness};
        for(std::size_t index=0;index<x_.size()+samples_.size();++index){
            Sample point{};
            if(index<x_.size())point={{static_cast<std::uint32_t>(index),0,0},{1,0,0}};
            else point=samples_[index-x_.size()];
            Vec3 start{},finish{};float inverseMass=0,freeWeight=0;
            for(unsigned k=0;k<3;++k){const auto v=point.ids[k];const float w=point.weights[k];start+=old_[v]*w;finish+=x_[v]*w;inverseMass+=invMass_[v]*w*w;if(invMass_[v]>0)freeWeight+=w;}
            // Samples extremely close to a kinematic seam have insufficient
            // free leverage and could otherwise fling a distant vertex.
            if(inverseMass<=0||freeWeight<.25f)continue;
            Box query;grow(query,start);grow(query,finish);query.lo-=padding;query.hi+=padding;
            std::array<unsigned,64> stack{};unsigned used=1;stack[0]=0;
            while(used){const auto& node=nodes_[stack[--used]];if(!overlaps(query,node.box))continue;
                if(node.count){for(unsigned j=node.begin;j<node.begin+node.count;++j){const auto& collider=colliders_[order_[j]];if(!overlaps(query,triangleBox(collider)))continue;
                    if(stats_.collisionTests>=config_.maxCollisionTests){stats_.budgetExceeded=true;return false;}
                    ++stats_.collisionTests;
                    finish={};for(unsigned k=0;k<3;++k)finish+=x_[point.ids[k]]*point.weights[k];
                    Vec3 correction{},normal{},velocity{};
                    if(contact(start,finish,collider,from,to,h,correction,normal,velocity)){
                        float largest=1;for(unsigned k=0;k<3;++k)largest=std::max(largest,invMass_[point.ids[k]]*point.weights[k]/inverseMass);
                        const float scale=std::min(1.f,4.f/largest);
                        for(unsigned k=0;k<3;++k){const auto v=point.ids[k];const float factor=invMass_[v]*point.weights[k]/inverseMass*scale;if(factor==0)continue;x_[v]+=correction*factor;contactNormal_[v]=normalized(contactNormal_[v]+normal,normal);contactVelocity_[v]=velocity;}
                    }
                }}else{stack[used++]=node.left;stack[used++]=node.right;}
            }
        }
        return true;
    }
    void solveSpring(Spring& spring,float h){
        const Vec3 difference=x_[spring.a]-x_[spring.b];const float distance=length(difference);
        if(distance<1.e-10f)return;
        const float wa=invMass_[spring.a],wb=invMass_[spring.b],alpha=spring.compliance/(h*h);
        if(wa+wb==0)return;
        const float lambda=(-(distance-spring.rest)-alpha*spring.lambda)/(wa+wb+alpha);
        spring.lambda+=lambda;const Vec3 delta=difference*(lambda/distance);
        x_[spring.a]+=delta*wa;x_[spring.b]-=delta*wb;
    }
    void solveArea(Face& face,float h){
        const auto t=face.tri;const Vec3 ab=x_[t.b]-x_[t.a],ac=x_[t.c]-x_[t.a],n=cross(ab,ac);
        const float twiceArea=length(n);if(twiceArea<1.e-10f)return;
        const Vec3 normal=n/twiceArea;
        const Vec3 ga=cross(x_[t.b]-x_[t.c],normal)*.5f,gb=cross(ac,normal)*.5f,gc=cross(normal,ab)*.5f;
        const float wa=invMass_[t.a],wb=invMass_[t.b],wc=invMass_[t.c],alpha=config_.areaCompliance/(h*h);
        const float denominator=wa*dot(ga,ga)+wb*dot(gb,gb)+wc*dot(gc,gc)+alpha;
        if(denominator<1.e-20f)return;
        const float lambda=(-(twiceArea*.5f-face.restArea)-alpha*face.lambda)/denominator;
        face.lambda+=lambda;x_[t.a]+=ga*(lambda*wa);x_[t.b]+=gb*(lambda*wb);x_[t.c]+=gc*(lambda*wc);
    }
    static bool cellLess(const Cell& a,const Cell& b){return a.x!=b.x?a.x<b.x:(a.y!=b.y?a.y<b.y:(a.z!=b.z?a.z<b.z:a.vertex<b.vertex));}
    void selfContacts(){
        // Local particle separation suppresses small folds. This is deliberately
        // NOT a claim of continuous triangle/triangle self collision: edges can
        // pass through faces between sparse mesh vertices.
        if(!config_.selfCollision||config_.selfThickness<=0)return;
        const float width=config_.selfThickness;
        cells_.clear();
        for(std::uint32_t i=0;i<x_.size();++i){const auto p=x_[i]/width;
            // Large world origins must not overflow a grid's signed integers.
            if(!finite(p)||std::fabs(p.x)>1.e8f||std::fabs(p.y)>1.e8f||std::fabs(p.z)>1.e8f)return;
            cells_.push_back({static_cast<int>(std::floor(p.x)),static_cast<int>(std::floor(p.y)),static_cast<int>(std::floor(p.z)),i});
        }
        std::sort(cells_.begin(),cells_.end(),cellLess);
        for(const auto& cell:cells_)for(int dx=-1;dx<=1;++dx)for(int dy=-1;dy<=1;++dy)for(int dz=-1;dz<=1;++dz){
            const Cell key{cell.x+dx,cell.y+dy,cell.z+dz,0};
            auto it=std::lower_bound(cells_.begin(),cells_.end(),key,cellLess);
            for(;it!=cells_.end()&&it->x==key.x&&it->y==key.y&&it->z==key.z;++it){
                const auto i=cell.vertex,j=it->vertex;if(j<=i)continue;
                if(stats_.selfPairs>=config_.maxSelfPairs)return;
                ++stats_.selfPairs;
                if(std::binary_search(adjacent_.begin(),adjacent_.end(),pair(i,j)))continue;
                const float wi=invMass_[i],wj=invMass_[j];if(wi+wj==0)continue;
                Vec3 delta=x_[i]-x_[j];const float distance=length(delta);if(distance>=width)continue;
                const Vec3 normal=normalized(delta,normalized(lastPose_[i]-lastPose_[j],{1,0,0}));
                delta=normal*((width-distance)/(wi+wj));x_[i]+=delta*wi;x_[j]-=delta*wj;
            }
        }
    }
    static bool validPosition(Vec3 p){
        // Bound intermediate float products and preserve useful local precision.
        // Callers operating beyond this world range must use a floating origin.
        return finite(p)&&std::fabs(p.x)<=1.e6f&&std::fabs(p.y)<=1.e6f&&std::fabs(p.z)<=1.e6f;
    }
    bool validPose(const std::vector<Vec3>& pose)const{if(pose.size()!=x_.size())return false;for(const auto p:pose)if(!validPosition(p))return false;return true;}
    bool finishVelocities(float h,bool collisionOnly=false){
        const float friction=1-std::pow(1-clamp(config_.friction,0,1),h/config_.fixedStep);
        for(std::size_t i=0;i<x_.size();++i){
            if(!validPosition(x_[i]))return false;
            const Vec3 normal=contactNormal_[i];
            if(collisionOnly){
                if(dot(normal,normal)<.1f)continue;
                velocity_[i]+=(x_[i]-old_[i])/h;
            }else velocity_[i]=(x_[i]-old_[i])/h;
            if(dot(normal,normal)>.1f&&invMass_[i]>0){
                Vec3 relative=velocity_[i]-contactVelocity_[i];const float inward=dot(relative,normal);
                if(inward<0)relative-=normal*inward;
                const Vec3 tangent=relative-normal*dot(relative,normal);
                velocity_[i]=relative-tangent*friction+contactVelocity_[i];
            }
            if(!finite(velocity_[i]))return false;
            const float speed=length(velocity_[i]);if(speed>config_.maxSpeed)velocity_[i]=velocity_[i]*(config_.maxSpeed/speed);
        }
        return true;
    }
    void resetState(const std::vector<Vec3>& pose){x_=pose;old_=pose;lastPose_=pose;std::fill(velocity_.begin(),velocity_.end(),Vec3{});accumulator_=0;stats_.reset=true;}
public:
    bool initialize(const std::vector<Vec3>& worldPositions,const std::vector<Triangle>& topology,const std::vector<std::uint32_t>& seamIndices,Config config={}){
        ready_=false;stats_={};config_=config;
        if(worldPositions.size()<3||worldPositions.size()>config.maxVertices||topology.empty()||topology.size()>config.maxTriangles||seamIndices.empty())return false;
        if(!finite(config.gravity)||!std::isfinite(config.fixedStep)||config.fixedStep<1.f/1000.f||config.fixedStep>1.f/30.f||config.maxSubsteps==0||config.maxSubsteps>32||config.iterations==0||config.iterations>32||config.maxVertices>65536||config.maxTriangles>131072||config.maxColliderTriangles>131072||config.maxContactSamples>8192)return false;
        const float positive[]={config.thickness,config.density,config.maxSpeed,config.teleportDistance,config.maxFrameTime,config.contactSpacing};for(float n:positive)if(!std::isfinite(n)||n<=0)return false;
        const float nonnegative[]={config.selfThickness,config.stretchCompliance,config.areaCompliance,config.bendCompliance,config.damping,config.friction};for(float n:nonnegative)if(!std::isfinite(n)||n<0)return false;
        for(auto p:worldPositions)if(!validPosition(p))return false;
        for(auto pin:seamIndices)if(pin>=worldPositions.size())return false;
        x_=worldPositions;old_=x_;lastPose_=x_;velocity_.assign(x_.size(),{});contactNormal_.resize(x_.size());contactVelocity_.resize(x_.size());invMass_.assign(x_.size(),0);
        pins_=seamIndices;std::sort(pins_.begin(),pins_.end());pins_.erase(std::unique(pins_.begin(),pins_.end()),pins_.end());
        stretch_.clear();bend_.clear();faces_.clear();adjacent_.clear();nodes_.clear();colliders_.clear();samples_.clear();cells_.reserve(x_.size());
        std::vector<Edge> edges;edges.reserve(topology.size()*3);
        for(auto t:topology){
            if(t.a>=x_.size()||t.b>=x_.size()||t.c>=x_.size()||t.a==t.b||t.b==t.c||t.c==t.a)return false;
            const float area=length(cross(x_[t.b]-x_[t.a],x_[t.c]-x_[t.a]))*.5f;
            if(!std::isfinite(area)||area<1.e-10f)return false;
            faces_.push_back({t,area,0});for(auto i:{t.a,t.b,t.c})invMass_[i]+=area*config.density/3;
            const std::uint32_t ids[3]={t.a,t.b,t.c};for(unsigned i=0;i<3;++i){auto a=ids[i],b=ids[(i+1)%3];if(a>b)std::swap(a,b);edges.push_back({a,b,ids[(i+2)%3]});}
        }
        for(float& mass:invMass_){if(mass<=0)return false;mass=1.f/mass;}
        for(auto i:pins_)invMass_[i]=0;
        std::sort(edges.begin(),edges.end(),[](const Edge&a,const Edge&b){return a.a!=b.a?a.a<b.a:(a.b!=b.b?a.b<b.b:a.opposite<b.opposite);});
        for(std::size_t i=0;i<edges.size();){std::size_t end=i+1;while(end<edges.size()&&edges[end].a==edges[i].a&&edges[end].b==edges[i].b)++end;
            const auto e=edges[i];stretch_.push_back({e.a,e.b,length(x_[e.a]-x_[e.b]),config.stretchCompliance,0});adjacent_.push_back(pair(e.a,e.b));
            // Non-manifold edges still stretch correctly; there is no unique
            // neighboring face pair to give such an edge a bending spring.
            if(end-i==2&&e.opposite!=edges[i+1].opposite){const auto b=edges[i+1].opposite;bend_.push_back({e.opposite,b,length(x_[e.opposite]-x_[b]),config.bendCompliance,0});adjacent_.push_back(pair(e.opposite,b));}
            i=end;
        }
        std::sort(adjacent_.begin(),adjacent_.end());adjacent_.erase(std::unique(adjacent_.begin(),adjacent_.end()),adjacent_.end());
        unsigned candidateCount=0;
        const auto appendSample=[&](Sample p){
            float free=0;for(unsigned k=0;k<3;++k)if(invMass_[p.ids[k]]>0)free+=p.weights[k];if(free<.25f)return;
            ++candidateCount;
            if(samples_.size()<config.maxContactSamples)samples_.push_back(p);
            else if(config.maxContactSamples){
                // Deterministic reservoir sampling spreads a capped budget
                // across the whole cape rather than only its first triangles.
                std::uint32_t hash=candidateCount;hash^=hash>>16;hash*=0x7feb352du;hash^=hash>>15;hash*=0x846ca68bu;hash^=hash>>16;
                const unsigned slot=hash%candidateCount;if(slot<samples_.size())samples_[slot]=p;
            }
        };
        for(const auto& edge:stretch_){const unsigned n=static_cast<unsigned>(clamp(std::ceil(edge.rest/config.contactSpacing),1,8));for(unsigned i=1;i<n;++i){const float t=float(i)/n;appendSample({{edge.a,edge.b,0},{1-t,t,0}});}}
        for(const auto& face:faces_){const auto t=face.tri;const float longest=std::max(length(x_[t.a]-x_[t.b]),std::max(length(x_[t.b]-x_[t.c]),length(x_[t.c]-x_[t.a])));const unsigned n=static_cast<unsigned>(clamp(std::ceil(longest/config.contactSpacing),1,8));for(unsigned i=1;i+1<n;++i)for(unsigned j=1;i+j<n;++j){const float u=float(i)/n,v=float(j)/n;appendSample({{t.a,t.b,t.c},{u,v,1-u-v}});}}
        accumulator_=0;ready_=true;return true;
    }
    // Returns false on invalid input or collision-budget exhaustion. A caller
    // can use its original animated cape for that frame; no bad state escapes.
    // Collider triangles must retain their vertex identities between previous
    // and current. They may be two-sided body, equipment, or world triangles.
    bool step(float elapsedSeconds,const std::vector<Vec3>& animatedWorldPositions,const std::vector<ColliderTriangle>& colliders){
        stats_={};if(!ready_)return false;
        if(!validPose(animatedWorldPositions)||!std::isfinite(elapsedSeconds)||elapsedSeconds<0){stats_.invalidInput=true;resetState(lastPose_);return false;}
        bool teleported=elapsedSeconds>config_.maxFrameTime;
        for(auto pin:pins_)if(length(animatedWorldPositions[pin]-lastPose_[pin])>config_.teleportDistance)teleported=true;
        if(teleported){resetState(animatedWorldPositions);return true;}
        if(colliders.size()>config_.maxColliderTriangles){stats_.budgetExceeded=true;resetState(animatedWorldPositions);return false;}
        for(const auto& triangle:colliders)for(unsigned i=0;i<3;++i)if(!validPosition(triangle.previous[i])||!validPosition(triangle.current[i])){stats_.invalidInput=true;resetState(animatedWorldPositions);return false;}
        colliders_=colliders;order_.resize(colliders_.size());for(unsigned i=0;i<order_.size();++i)order_[i]=i;nodes_.clear();nodes_.reserve(colliders_.size()*2);if(!colliders_.empty())buildNode(0,static_cast<unsigned>(colliders_.size()));
        const double before=accumulator_;accumulator_+=elapsedSeconds;
        const double requestedSteps=std::floor((accumulator_+1.e-9)/config_.fixedStep);
        if(requestedSteps>config_.maxSubsteps){stats_.budgetExceeded=true;resetState(animatedWorldPositions);return false;}
        const unsigned steps=static_cast<unsigned>(requestedSteps);
        const float h=config_.fixedStep,decay=std::exp(-config_.damping*h);
        for(unsigned stepIndex=0;stepIndex<steps;++stepIndex){
            const float from=elapsedSeconds>0?clamp(static_cast<float>(stepIndex*h-before)/elapsedSeconds,0,1):1;
            const float to=elapsedSeconds>0?clamp(static_cast<float>((stepIndex+1)*h-before)/elapsedSeconds,0,1):1;
            old_=x_;std::fill(contactNormal_.begin(),contactNormal_.end(),Vec3{});std::fill(contactVelocity_.begin(),contactVelocity_.end(),Vec3{});
            for(std::size_t i=0;i<x_.size();++i)if(invMass_[i]>0){velocity_[i]=(velocity_[i]+config_.gravity*h)*decay;x_[i]+=velocity_[i]*h;}
            for(auto pin:pins_)x_[pin]=lerp(lastPose_[pin],animatedWorldPositions[pin],to);
            for(auto& spring:stretch_)spring.lambda=0;
            for(auto& spring:bend_)spring.lambda=0;
            for(auto& face:faces_)face.lambda=0;
            for(unsigned iteration=0;iteration<config_.iterations;++iteration){
                for(auto& spring:stretch_)solveSpring(spring,h);
                for(auto& face:faces_)solveArea(face,h);
                for(auto& spring:bend_)solveSpring(spring,h);
                if(iteration+1==config_.iterations)selfContacts();
                if(iteration==0||iteration+1==config_.iterations)if(!collisions(from,to,h)){resetState(animatedWorldPositions);return false;}
            }
            if(!finishVelocities(h)){stats_.invalidInput=true;resetState(animatedWorldPositions);return false;}
            ++stats_.substeps;
        }
        accumulator_-=steps*static_cast<double>(h);if(accumulator_<0)accumulator_=0;
        // Even without a full physics tick, a fast animated collider can sweep
        // completely through a particle. Resolve the unsimulated part of this
        // display frame immediately; no collider path is discarded by the
        // accumulator. Free flight/constraint integration still uses fixed h.
        if(!nodes_.empty()&&elapsedSeconds>0&&accumulator_>1.e-9){
            const float from=steps?clamp(static_cast<float>(steps*h-before)/elapsedSeconds,0,1):0;
            const float tailTime=(1-from)*elapsedSeconds;
            if(tailTime>1.e-8f){
                old_=x_;std::fill(contactNormal_.begin(),contactNormal_.end(),Vec3{});std::fill(contactVelocity_.begin(),contactVelocity_.end(),Vec3{});
                if(!collisions(from,1,tailTime)){resetState(animatedWorldPositions);return false;}
                if(!finishVelocities(tailTime,true)){stats_.invalidInput=true;resetState(animatedWorldPositions);return false;}
            }
        }
        // The visible seam follows every display frame, including frames which
        // do not accumulate a full physics tick.
        for(auto pin:pins_)x_[pin]=animatedWorldPositions[pin];
        lastPose_=animatedWorldPositions;return true;
    }
    void reset(const std::vector<Vec3>& animatedWorldPositions){if(ready_&&validPose(animatedWorldPositions))resetState(animatedWorldPositions);}
    bool ready()const{return ready_;}
    const std::vector<Vec3>& positions()const{return x_;}
    const Stats& stats()const{return stats_;}
    std::size_t collisionSampleCount()const{return x_.size()+samples_.size();}
    const Config& config()const{return config_;}
};
} // namespace cape
