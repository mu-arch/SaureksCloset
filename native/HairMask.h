#pragma once
#include "CapeMotion.h"
#include "BagCoordinates.h"
#include <algorithm>
#include <mutex>

// A private, topology-preserving fit. Never cut the character's scalp, delete
// faces, or mutate a shared live model. All calculations use bind-pose space.
namespace hairMask {
using capeMotion::Bytes;
using capeMotion::u16;using capeMotion::u32;using capeMotion::range;
using Point=std::array<float,3>;
using Triangle=std::array<Point,3>;
inline float number(const Bytes& b,unsigned p){float f;std::memcpy(&f,b.data()+p,4);return f;}
inline Point transform(const BagMatrix& m,const Point& p){Point q{};for(unsigned i=0;i<3;++i)q[i]=m[12+i]+m[i]*p[0]+m[4+i]*p[1]+m[8+i]*p[2];return q;}
inline Point sub(const Point& a,const Point& b){return {{a[0]-b[0],a[1]-b[1],a[2]-b[2]}};}
inline Point cross(const Point& a,const Point& b){return {{a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]}};}
inline float dot(const Point& a,const Point& b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
inline bool finite(const Point& p){for(auto v:p)if(!std::isfinite(v))return false;return true;}
// Ray parameter, independent of triangle winding. Open hats need not form a
// watertight solid. Only actual opaque triangles count as coverage.
inline float hit(const Point& origin,const Point& direction,const Triangle& triangle){
    const auto e1=sub(triangle[1],triangle[0]),e2=sub(triangle[2],triangle[0]);
    const auto h=cross(direction,e2);const float det=dot(e1,h);if(std::fabs(det)<1e-10f)return -1;
    const float inv=1.f/det;const auto s=sub(origin,triangle[0]);const float u=dot(s,h)*inv;if(u< -1e-5f||u>1.00001f)return -1;
    const auto q=cross(s,e1);const float v=dot(direction,q)*inv;if(v< -1e-5f||u+v>1.00001f)return -1;
    const float t=dot(e2,q)*inv;return t>1e-5f?t:-1;
}
inline float nearest(const Point& origin,const Point& direction,const std::vector<Triangle>& mesh){
    float nearest=1e10f;for(const auto& triangle:mesh){const auto t=hit(origin,direction,triangle);if(t>0)nearest=std::min(nearest,t);}return nearest;
}
// Geoset IDs alone are not a material classification: several hairstyles have
// skin-textured scalp patches in the SAME geoset. Every batch must use hair.
inline bool hairSection(const Bytes& b,unsigned view,unsigned section){
    const auto count=u32(b,view+32),at=u32(b,view+36),nt=u32(b,92),textures=u32(b,96),nl=u32(b,148),lookup=u32(b,152);
    if(count>2048||!range(b,at,count,24)||!range(b,textures,nt,16)||!range(b,lookup,nl,2))return false;
    bool found=false;
    for(unsigned i=0;i<count;++i){const auto batch=at+24*i;if(u16(b,batch+4)!=section)continue;
        found=true;const auto start=u16(b,batch+16),n=u16(b,batch+14);if(!n||start+n>nl)return false;
        for(unsigned j=0;j<n;++j){const auto texture=u16(b,lookup+2*(start+j));if(texture>=nt||u32(b,textures+16*texture)!=6)return false;}
    }
    return found;
}
inline bool build(const Bytes& base,unsigned hairGroup,const std::vector<Triangle>& hat,const BagMatrix& modelToHat,Bytes& result,unsigned& changed){
    changed=0;BagMatrix hatToModel;if(!bagAffineInverse(modelToHat,hatToModel)||hat.size()>20000)return false;
    if(base.size()<0x144||u32(base,0)!=0x3032444d||u32(base,4)!=256||hairGroup<1||hairGroup>=100)return false;
    const auto nv=u32(base,68),vo=u32(base,72),views=u32(base,76),viewOffset=u32(base,80);
    if(!nv||nv>=65536||!range(base,vo,nv,48)||!views||views>16||!range(base,viewOffset,views,44))return false;
    std::vector<Triangle> surface;surface.reserve(hat.size());
    for(const auto& t:hat){Triangle rest;for(unsigned j=0;j<3;++j){if(!finite(t[j]))return false;rest[j]=transform(hatToModel,t[j]);if(!finite(rest[j]))return false;}surface.push_back(rest);}
    std::vector<bool> eligible(nv,false),protectedVertex(nv,false);std::vector<Triangle> scalp,skin;
    for(unsigned view=0;view<views;++view){
        const auto a=viewOffset+view*44,ni=u32(base,a),io=u32(base,a+4),nt=u32(base,a+8),to=u32(base,a+12),ns=u32(base,a+24),so=u32(base,a+28);
        if(ni>65535||nt>65535||ns>512||!range(base,io,ni,2)||!range(base,to,nt,2)||!range(base,so,ns,32))return false;
        for(unsigned s=0;s<ns;++s){const auto section=so+32*s,group=u16(base,section),first=u16(base,section+8),count=u16(base,section+10);
            if(first+count>nt||count%3)return false;
            const bool hair=group==hairGroup&&hairSection(base,a,s);
            for(unsigned t=first;t<first+count;t+=3){Triangle tri;
                for(unsigned j=0;j<3;++j){const auto ix=u16(base,to+2*(t+j));if(ix>=ni)return false;const auto vertex=u16(base,io+2*ix);if(vertex>=nv)return false;
                    (hair?eligible:protectedVertex)[vertex]=true;
                    for(unsigned k=0;k<3;++k)tri[j][k]=number(base,vo+48*vertex+4*k);if(!finite(tri[j]))return false;
                }
                // The stock bald scalp gives a head reference without guessing
                // a hat's origin, crown height, or a race-specific cutting plane.
                if(view==0&&group==1&&!hairSection(base,a,s))scalp.push_back(tri);
                if(view==0&&(group==0||(group==hairGroup&&!hair)))skin.push_back(tri);
            }
        }
    }
    result=base;if(scalp.empty()||surface.empty())return true;
    Point lo=scalp[0][0],hi=lo;
    for(const auto& tri:scalp)for(const auto& p:tri)for(unsigned k=0;k<3;++k){lo[k]=std::min(lo[k],p[k]);hi[k]=std::max(hi[k],p[k]);}
    Point center;for(unsigned k=0;k<3;++k)center[k]=(lo[k]+hi[k])*.5f;
    const float clearance=std::max(.002f,std::min(.01f,(hi[2]-lo[2])*.1f));
    for(unsigned v=0;v<nv;++v){if(!eligible[v]||protectedVertex[v])continue;
        Point p;for(unsigned k=0;k<3;++k)p[k]=number(base,vo+v*48+k*4);
        // Keep hanging lengths/ponytails below the crown. An open side without
        // real hat coverage is also untouched.
        if(p[2]<lo[2]-(hi[2]-lo[2])*.4f)continue;
        const auto ray=sub(p,center);const float length=std::sqrt(dot(ray,ray));if(length<clearance)continue;
        const float t=nearest(center,ray,surface);if(t>=1.f||t<=0)continue;
        const float skull=nearest(center,ray,scalp),visibleSkin=nearest(center,ray,skin),padding=clearance/length;
        // A hat inside the head is an impossible fit, not permission to erase
        // skin or push hair through the scalp. Leave that region intact.
        // The bald cap is a reference, not necessarily visible with this
        // hairstyle. Enforce actual visible skin, not a hidden bald mesh.
        if((skull<1e9f&&t<=skull*.5f)||t<=.25f||t<=2*padding||
           (visibleSkin<1e9f&&t<=visibleSkin+.0005f/length))continue;
        const float margin=visibleSkin<1e9f?std::min(padding,(t-visibleSkin)*.4f):padding;
        const float fit=t-margin;
        Point target;for(unsigned k=0;k<3;++k)target[k]=center[k]+ray[k]*fit;
        std::memcpy(result.data()+vo+v*48,target.data(),12);++changed;
    }
    // Triangle/index buffers, skin geometry, UVs, normals and skinning weights
    // are byte-for-byte original. Preserving topology prevents open cut seams.
    return true;
}
// Exact allowlist of files generated by this process, never arbitrary paths.
inline std::map<std::string,std::string> generated;
inline std::mutex generatedMutex;
inline bool cachePath(const char* path){if(!path)return false;std::lock_guard<std::mutex> lock(generatedMutex);for(const auto& e:generated)if(capeMotion::pathEqual(e.second.c_str(),path))return true;return false;}
inline const char* save(const Bytes& bytes){
    // The native asset loader can query the allowlist on its worker thread.
    std::lock_guard<std::mutex> lock(generatedMutex);
    char key[32];std::snprintf(key,sizeof(key),"%08x_%08x",capeMotion::crc(bytes),unsigned(bytes.size()));auto it=generated.find(key);if(it!=generated.end())return it->second.c_str();
    const char* dir="Interface/AddOns/SaureksCloset/CapeMotion/Cache";
#ifdef _WIN32
    _mkdir(dir);
#else
    mkdir(dir,0755);
#endif
    const std::string path=std::string(dir)+"/H1_"+key+".m2";Bytes existing;
    if(!capeMotion::readFile(path,existing)||existing!=bytes){const auto tmp=path+".tmp";{std::ofstream f(tmp,std::ios::binary|std::ios::trunc);if(!f||!f.write(reinterpret_cast<const char*>(bytes.data()),bytes.size()))return nullptr;}if(std::rename(tmp.c_str(),path.c_str())){std::remove(tmp.c_str());return nullptr;}}
    return generated.emplace(key,path).first->second.c_str();
}
inline void (*capture)(void*,const float*)=nullptr;
inline std::string activePath;
inline std::uint64_t activeOwner=0;
inline unsigned activeBody=0,activeStyle=0;
inline const char* model(const char* original,std::uint64_t owner,unsigned body,unsigned style){return owner&&owner==activeOwner&&body==activeBody&&style==activeStyle&&!activePath.empty()?activePath.c_str():original;}
}
