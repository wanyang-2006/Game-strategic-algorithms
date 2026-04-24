#ifndef TUNING_PARAMS_H
#define TUNING_PARAMS_H

namespace Tuning {
    // MVV 
    // 对应 stoneType: None=0, King=1, Bishop=2, Knight=3, Rook=4, Pawn=5, Cannon=6, Assistant=7
    constexpr int VictimValue[8] = {
        0,      // None
        10000,  // King 
        200,    // Bishop 
        400,    // Knight 
        900,    // Rook 
        100,    // Pawn 
        450,    // Cannon 
        200     // Assistant 
    };

    // LVA 
    constexpr int AttackerValue[8] = {
        0,      // None
        10000,  // King
        200,    // Bishop
        400,    // Knight
        900,    // Rook
        100,    // Pawn
        450,    // Cannon
        200     // Assistant
    };
    
    // 吃子着法的得分永远高于非吃子着法
    constexpr int CaptureBaseScore = 10000;

    inline int getMVVLVAScore(stoneType attacker, stoneType victim) {
    if (victim == None) {
        return 0;
    }//MVV/LAA
    
    return Tuning::CaptureBaseScore + (Tuning::VictimValue[victim] * 10) - Tuning::AttackerValue[attacker];
}
}

#endif // TUNING_PARAMS_H