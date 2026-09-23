// Generated numeric staff envelope over model-local X[-1.0,0.15].
// Triangle edges are clipped to this torso contact band; contained
// vertices retain lower-shaft flares instead of measuring only x=0.
// Verify with tools/audit_staff_shafts.py against build 5875.
// Paths and four bounds only; no client model geometry.
#pragma once
#include <cstring>
struct StaffShaft { const char* model; float minY,maxY,minZ,maxZ; };
static constexpr StaffShaft staffShafts[]={
    {"Item\\ObjectComponents\\Weapon\\Misc_2H_Broom_A_01.mdx",-0.020174653f,0.033856501f,-0.001787784f,0.053281175f},
    {"Item\\ObjectComponents\\Weapon\\Stave_2H_AhnQiraj_D_01.mdx",-0.055084538f,0.055691976f,-0.053033546f,0.055603199f},
    {"Item\\ObjectComponents\\Weapon\\Stave_2H_AhnQiraj_D_02.mdx",-0.054219183f,0.054417554f,-0.053765643f,0.054871082f},
    {"Item\\ObjectComponents\\Weapon\\Stave_2H_AhnQiraj_D_03.mdx",-0.069831766f,0.070050374f,-0.089998859f,0.088017714f},
    {"Item\\ObjectComponents\\Weapon\\Stave_2H_AhnQiraj_D_04.mdx",-0.030720364f,0.030839817f,-0.031551238f,0.030008887f},
    {"Item\\ObjectComponents\\Weapon\\Stave_2H_BlackWIng_A_02.mdx",-0.068098523f,0.067590341f,-0.066969655f,0.067838728f},
    {"Item\\ObjectComponents\\Weapon\\Stave_2H_BlackWing_A_01.mdx",-0.024461545f,0.024600322f,-0.010943366f,0.045289285f},
    {"Item\\ObjectComponents\\Weapon\\Stave_2H_Epic_A_01.mdx",-0.051826436f,0.054301240f,-0.059736948f,0.059736617f},
    {"Item\\ObjectComponents\\Weapon\\Stave_2H_Flaming_D_01.mdx",-0.024441294f,0.024685801f,-0.004858359f,0.053016428f},
    {"Item\\ObjectComponents\\Weapon\\Stave_2H_Jeweled_A_01.mdx",-0.024448567f,0.025088714f,-0.029406031f,0.026869539f},
    {"Item\\ObjectComponents\\Weapon\\Stave_2H_Jeweled_A_02.mdx",-0.048415475f,0.048547007f,-0.054155149f,0.055416763f},
    {"Item\\ObjectComponents\\Weapon\\Stave_2H_Jeweled_A_03.mdx",-0.026048085f,0.027330719f,-0.030607786f,0.029712591f},
    {"Item\\ObjectComponents\\Weapon\\Stave_2H_Jeweled_B_01.mdx",-0.024367587f,0.025182031f,-0.030179554f,0.026142469f},
    {"Item\\ObjectComponents\\Weapon\\Stave_2H_Jeweled_B_02.mdx",-0.024162392f,0.025365615f,-0.027839231f,0.028401557f},
    {"Item\\ObjectComponents\\Weapon\\Stave_2H_Jeweled_C_01.mdx",-0.028243143f,0.027056462f,-0.030683471f,0.033622137f},
    {"Item\\ObjectComponents\\Weapon\\Stave_2H_Jeweled_D_01.mdx",-0.024829847f,0.024302146f,-0.030805221f,0.027069299f},
    {"Item\\ObjectComponents\\Weapon\\Stave_2H_Long_A_01.mdx",-0.032864660f,0.059145153f,-0.116051998f,0.048360389f},
    {"Item\\ObjectComponents\\Weapon\\Stave_2H_Long_A_02.mdx",-0.048627350f,0.048335135f,-0.054966468f,0.054605443f},
    {"Item\\ObjectComponents\\Weapon\\Stave_2H_Long_A_03.mdx",-0.024497746f,0.025028988f,-0.030003414f,0.026213355f},
    {"Item\\ObjectComponents\\Weapon\\Stave_2H_Long_A_04.mdx",-0.047833076f,0.034176655f,-0.051184613f,0.088417940f},
    {"Item\\ObjectComponents\\Weapon\\Stave_2H_Long_B_01.mdx",-0.048346855f,0.048615631f,-0.054405410f,0.055166487f},
    {"Item\\ObjectComponents\\Weapon\\Stave_2H_Long_B_02Holy.mdx",-0.031043410f,0.031744260f,-0.031267703f,0.031519976f},
    {"Item\\ObjectComponents\\Weapon\\Stave_2H_Long_B_03.mdx",-0.037839491f,0.039114874f,-0.044375569f,0.042586256f},
    {"Item\\ObjectComponents\\Weapon\\Stave_2H_Long_B_04.mdx",-0.041334794f,0.032492615f,-0.049646575f,0.079217412f},
    {"Item\\ObjectComponents\\Weapon\\Stave_2H_Long_C_01.mdx",-0.023771240f,0.025761453f,-0.029448563f,0.026809756f},
    {"Item\\ObjectComponents\\Weapon\\Stave_2H_Long_C_02.mdx",-0.022834988f,0.026697706f,-0.027738418f,0.028519848f},
    {"Item\\ObjectComponents\\Weapon\\Stave_2H_Long_D_01.mdx",-0.034105529f,0.034722486f,-0.039862487f,0.040588204f},
    {"Item\\ObjectComponents\\Weapon\\Stave_2H_Long_D_05.mdx",-0.178566128f,0.179031894f,-0.139537171f,0.139103413f},
    {"Item\\ObjectComponents\\Weapon\\Stave_2H_Long_EpicPriest01.mdx",-0.061037939f,0.061529048f,-0.070461392f,0.070206076f},
    {"Item\\ObjectComponents\\Weapon\\Stave_2H_Long_EpicPriest02.mdx",-0.102536887f,0.102897659f,-0.118572660f,0.116904579f},
    {"Item\\ObjectComponents\\Weapon\\Stave_2H_Medivh_D_01.mdx",-0.028719118f,0.029271351f,-0.033689922f,0.032463681f},
    {"Item\\ObjectComponents\\Weapon\\Stave_2H_Other_A_01.mdx",-0.028580862f,0.030606223f,-0.113497831f,0.031784426f},
    {"Item\\ObjectComponents\\Weapon\\Stave_2H_Other_B_01.mdx",-0.028005045f,0.042299021f,-0.032368619f,0.057590971f},
    {"Item\\ObjectComponents\\Weapon\\Stave_2H_Other_C_01.mdx",-0.025020571f,0.024505307f,-0.027685422f,0.028547409f},
    {"Item\\ObjectComponents\\Weapon\\Stave_2H_Other_D_01.mdx",-0.018237563f,0.031288314f,-0.010943340f,0.045289365f},
    {"Item\\ObjectComponents\\Weapon\\Stave_2H_PVPAlliance_A_01.mdx",-0.041136727f,0.072493747f,-0.194402307f,0.049201403f},
    {"Item\\ObjectComponents\\Weapon\\Stave_2H_PVPHorde_A_01.mdx",-0.061358809f,0.074542843f,-0.196665764f,0.071199350f},
    {"Item\\ObjectComponents\\Weapon\\Stave_2H_Scythe_C_03.mdx",-0.036205804f,0.021780218f,-0.038400838f,0.061171011f},
    {"Item\\ObjectComponents\\Weapon\\Stave_2H_Stratholme_D_01.mdx",-0.131625697f,0.132739499f,-0.177157119f,0.067472376f},
    {"Item\\ObjectComponents\\Weapon\\Stave_2H_Stratholme_D_02.mdx",-0.139954284f,0.139954284f,-0.086141348f,0.045755420f},
    {"Item\\ObjectComponents\\Weapon\\Stave_2H_Stratholme_D_03.mdx",-0.067825884f,0.067891695f,-0.109388754f,0.026204763f},
    {"Item\\ObjectComponents\\Weapon\\Stave_2H_Zulgurub_D_01.mdx",-0.119766504f,0.100269884f,-0.097290911f,0.108814687f},
    {"Item\\ObjectComponents\\Weapon\\Stave_2H_Zulgurub_D_02.mdx",-0.083379641f,0.064064562f,-0.068385638f,0.083775401f},
    {"Item\\ObjectComponents\\Weapon\\Stave_2H_Zulgurub_D_03.mdx",-0.031364019f,0.036560174f,-0.071276766f,0.074879711f},
};
inline const StaffShaft* staffShaftFor(const char* model){
    if(!model)return nullptr;
    for(const auto& shaft:staffShafts)
        if(std::strcmp(model,shaft.model)==0)return &shaft;
    return nullptr;
}
