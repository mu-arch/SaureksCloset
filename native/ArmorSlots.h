#pragma once
// Build 5875: 478CB0 maps zero-based inventory slots through 478DA4/478D74.
// Inventory 14 (cape) dispatches 478D63 -> component slot 10; inventory 18
// (tabard) dispatches 478D54 -> slot 9. Component rows start at +4A8.
inline constexpr int armorComponentSlot(unsigned inventorySlot){
    switch(inventorySlot){
        case 1:return 0;case 3:return 1;case 4:return 2;case 5:return 3;
        case 6:return 4;case 7:return 5;case 8:return 6;case 9:return 7;
        case 10:return 8;case 15:return 10;case 19:return 9;
        default:return -1;
    }
}
