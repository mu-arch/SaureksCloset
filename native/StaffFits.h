// Generated numeric sheathed-staff contact fits; no client model geometry.
// Verify with tools/audit_staff_fits.py against build 5875.
// Neutral-space +X points inward. Scan the authored shaft across the torso:
// grip Z -0.6..+0.04, patches Y +/-0.02 and Z +/-0.04, shaft step 0.01.
// Minimum clearance reaches the rear body surface; subtract shaft radius/gap.
#pragma once
#include <array>
struct StaffFit { unsigned race,sex,point; std::array<float,3> anchor; float inward; };
static constexpr StaffFit staffFits[]={
    {1,0,30,{{-0.300322115f,0.096468702f,1.514281988f}},0.132053672f}, // Human Male
    {1,0,31,{{-0.214352280f,-0.098810129f,1.519540310f}},0.014841447f}, // Human Male
    {1,1,30,{{-0.227966756f,0.071834251f,1.521607876f}},0.122281672f}, // Human Female
    {1,1,31,{{-0.161630958f,-0.071786955f,1.521616578f}},0.029868213f}, // Human Female
    {2,0,30,{{-0.380372912f,0.096468709f,1.858240128f}},0.149628233f}, // Orc Male
    {2,0,31,{{-0.302147716f,-0.098810129f,1.857316732f}},0.022767070f}, // Orc Male
    {2,1,30,{{-0.147560805f,0.071834102f,1.668498874f}},0.080514754f}, // Orc Female
    {2,1,31,{{-0.081319444f,-0.071787111f,1.672037005f}},0.002205609f}, // Orc Female
    {3,0,30,{{-0.288512230f,0.082472220f,1.247923374f}},0.094682569f}, // Dwarf Male
    {3,0,31,{{-0.223102465f,-0.082471728f,1.236878157f}},0.003539855f}, // Dwarf Male
    {3,1,30,{{-0.329180598f,0.082472205f,1.144717813f}},0.022737936f}, // Dwarf Female
    {3,1,31,{{-0.263770878f,-0.082471751f,1.133672595f}},-0.117685938f}, // Dwarf Female
    {4,0,30,{{-0.347614557f,0.093583331f,1.871585608f}},0.141288319f}, // NightElf Male
    {4,0,31,{{-0.263639957f,-0.093576692f,1.871585846f}},-0.043280549f}, // NightElf Male
    {4,1,30,{{-0.239098847f,0.071834207f,1.858304977f}},0.078031826f}, // NightElf Female
    {4,1,31,{{-0.172930345f,-0.071787030f,1.863012075f}},0.003182547f}, // NightElf Female
    {5,0,30,{{-0.348944336f,0.096468665f,1.666872501f}},0.067838990f}, // Scourge Male
    {5,0,31,{{-0.273525387f,-0.098810174f,1.647266865f}},-0.006850117f}, // Scourge Male
    {5,1,30,{{-0.167116478f,0.101429559f,1.519062757f}},-0.021184271f}, // Scourge Female
    {5,1,31,{{-0.128761247f,-0.100104667f,1.519329548f}},-0.135129452f}, // Scourge Female
    {6,0,30,{{-0.434773594f,0.096468702f,1.787553191f}},-0.021593662f}, // Tauren Male
    {6,0,31,{{-0.356683701f,-0.098810129f,1.792244554f}},-0.109815588f}, // Tauren Male
    {6,1,30,{{-0.369857311f,0.096464574f,1.764010787f}},0.064159545f}, // Tauren Female
    {6,1,31,{{-0.268785268f,-0.082147524f,1.771817207f}},-0.037112172f}, // Tauren Female
    {7,0,30,{{-0.182121471f,0.104166664f,0.722222149f}},0.003183362f}, // Gnome Male
    {7,0,31,{{-0.145833343f,-0.104166664f,0.722222149f}},-0.012746743f}, // Gnome Male
    {7,1,30,{{-0.226089343f,0.101742044f,0.615277708f}},0.024311913f}, // Gnome Female
    {7,1,31,{{-0.170833349f,-0.031941101f,0.624999940f}},-0.009992788f}, // Gnome Female
    {8,0,30,{{-0.350046188f,0.093583502f,2.340083361f}},0.085143221f}, // Troll Male
    {8,0,31,{{-0.274676263f,-0.093576513f,2.339972019f}},0.022657858f}, // Troll Male
    {8,1,30,{{-0.208333388f,0.111111112f,2.013888836f}},0.067947504f}, // Troll Female
    {8,1,31,{{-0.152777806f,-0.111111097f,2.013889074f}},-0.013279798f}, // Troll Female
};
// SHA-256 of the local source models:
// Human Male: f85bd8f3aa476f8add6975957d65e1bb19ad9f783c80a779028e88879a0d5389
// Human Female: 3b41e97da790ef8d8124c9c0bfa60b419160628e85232e130a3e1ab0213031fe
// Orc Male: f5f7bf20b8b75d29f11ac1df38183f0e02fc0da10675d3145c14de781114523e
// Orc Female: a15ce6de5212287e53bddade995d6c8b2e85d9ef974d40378f86017361d9a4d4
// Dwarf Male: 84e0e8c61bb8dd619f58dc897a7bf132a72d2a5e98d8c1cfd2773ccafa5ccf5e
// Dwarf Female: 953623bb594d469827724f091c7dcf499a466195aa9167717cfe17e1fdf34e9d
// NightElf Male: 077ffb9788c318d85b228eddaea418445478e2d1da4425a1c8a204846970113f
// NightElf Female: b139f5b1246e1edb9dee87ca752506045727f7a8377b3e6c1a6288573a1d96fb
// Scourge Male: c1140f444b6db79f949fb1ee2293aec3eff62a04d1ea07c8cdfa75f976d8dc5a
// Scourge Female: 4ab70aa515e9287d101ac6f54ec99dc8dca812c9365237be735e37baf5c1a648
// Tauren Male: 65f1dbb9621e6a2c64642abbf961afa0971b1d3177d84c157575c96569b6ad80
// Tauren Female: a98d3cac3d393cc60dc3028cc5be02b4a09aa8ded163c4687b7aac9d7e3e4800
// Gnome Male: 49d8582f0a52952ac04c847ffb32d523985aece1ccc3ad4bcc3a95035d259374
// Gnome Female: dfca5479cf6dc85d0b4fb0bbd12c77c0849603699c87aa546ab1e37f690f0941
// Troll Male: a3b1d54bf6c310d264aa85a383ca5c7f49b23f9bf5af99dc9a902a87f79a2ef8
// Troll Female: db3388e12559483fc7ef95c0dfe34e3b68128e55a45359c848554107369540cc
