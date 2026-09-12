#pragma once
// Build 5875 CM2 attachment layout. Read the active model, not the player's
// original race: wardrobe previews and race overrides have their own skeletons.
// Called only during recursive child update, after the parent bones are ready.
static bool animatedAttachmentMatrix(std::uintptr_t parent,unsigned point,std::array<float,16>& matrix){
    std::uintptr_t data=0,header=0,lookup=0,attachments=0,bones=0;
    unsigned lookupCount=0,attachmentCount=0,boneCount=0,id=0;
    std::uint16_t index=0,bone=0;
    if(!read(parent+0x30,data)||!data||!read(data+0x130,header)||!header||
       !read(header+0x10C,lookupCount)||lookupCount<=point||lookupCount>512||
       !read(header+0x110,lookup)||!lookup||!read(lookup+2*point,index)||
       !read(header+0x104,attachmentCount)||attachmentCount>512||index>=attachmentCount||
       !read(header+0x108,attachments)||!attachments)return false;
    const auto record=attachments+48*index;
    std::array<float,3> local;
    if(!read(record,id)||id!=point||!read(record+4,bone)||
       !read(header+0x34,boneCount)||boneCount>2048||bone>=boneCount||
       !read(record+8,local)||!read(parent+0x94,bones)||!bones||
       !read(bones+64*bone,matrix))return false;
    for(auto value:local)if(!std::isfinite(value))return false;
    for(auto value:matrix)if(!std::isfinite(value))return false;
    if(std::fabs(matrix[15]-1.f)>.001f)return false;
    // Same bone-space -> parent-space calculation as 0x7186A6..0x718756.
    // Do not multiply by the parent or scene matrix a second time.
    for(unsigned axis=0;axis<3;++axis){
        matrix[12+axis]=matrix[12+axis]+local[0]*matrix[axis]+
            local[1]*matrix[4+axis]+local[2]*matrix[8+axis];
        if(!std::isfinite(matrix[12+axis]))return false;
    }
    return true;
}
static bool animatedBackPosition(std::uintptr_t parent,std::array<float,3>& position){
    std::array<float,16> matrix;
    if(!animatedAttachmentMatrix(parent,28,matrix))return false;
    for(unsigned axis=0;axis<3;++axis)position[axis]=matrix[12+axis];
    return true;
}
