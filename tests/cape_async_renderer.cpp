#define SAUREKS_CAPE_ASYNC_TEST
#define main synchronousRendererMain
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
#include "cape_renderer.cpp"
#pragma GCC diagnostic pop
#undef main
int main(){
    for(bool gpu:{true,false}){
        fixture(gpu);const auto calls=submits;
        capeSubmitHook(nullptr,1);assert(capeStatus()==5&&submits==calls+1);
        // A duplicate material pass must retain 'preparing', not flicker to
        // unsupported just because that millisecond was already scheduled.
        capeSubmitHook(nullptr,1);assert(capeStatus()==5&&submits==calls+2);
        cape::waitCapeWorkerForTests();float motion=0;
        for(unsigned frame=0;frame<90;++frame){
            auto matrix=identity;matrix[12]=std::sin(frame*.1f)*.06f;
            put(0x80000,matrix);put(0x100fc,matrix);now+=17;
            const auto before=submits;capeSubmitHook(nullptr,1);
            assert(capeStatus()==2&&submits==before+1&&bound==&nativeBuffer);
            assert(capeFabricFits(capeState.cloth.positions(),capeState.animated,capeState.cloth.material(),capeState.triangles,capeState.pins));
            for(unsigned i=0;i<capeState.animated.size();++i)motion=std::max(motion,cape::length(capeState.cloth.positions()[i]-capeState.animated[i]));
            cape::waitCapeWorkerForTests();
        }
        assert(motion>.005f);
        // Reset invalidates old solver output without joining its thread.
        cape::delayCapeWorkerForTests(100);now+=17;capeSubmitHook(nullptr,1);
        capeReset();now+=17;capeSubmitHook(nullptr,1);assert(capeStatus()==5);
        cape::waitCapeWorkerForTests();cape::delayCapeWorkerForTests(0);
        now+=17;capeSubmitHook(nullptr,1);assert(capeStatus()==2);
        capeSetEnabled(false);cape::waitCapeWorkerForTests();
    }
    std::cout<<"async native renderer: preparing status across material passes, GPU/CPU output, real worker motion and reset isolation passed\n";
}
