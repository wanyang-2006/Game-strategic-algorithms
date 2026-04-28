#ifndef TUNING_PARAMS_H
#define TUNING_PARAMS_H

namespace Tuning {
    // MVV 
    // 对应 stoneType: None=0, King=1, Bishop=2, Knight=3, Rook=4, Pawn=5, Cannon=6, Assistant=7
    constexpr int VictimValue[8] = {
        0,      // None
        5,  // King 
        1,    // Bishop 
        3,   // Knight 
        4,    // Rook 
        2,    // Pawn 
        3,    // Cannon 
        1     // Assistant 
    };

    // LVA 
    constexpr int AttackerValue[8] = {
        0,      // None
        5,  // King
        1,    // Bishop
        3,    // Knight
        4,    // Rook
        2,    // Pawn
        3,    // Cannon
        1     // Assistant
    };
    
    // 吃子着法的得分永远高于非吃子着法
    constexpr int CaptureBaseScore = 100;

    inline int getMVVLVAScore(stoneType attacker, stoneType victim, bool isProtected) {
    if (victim == None) {
        return 0;
    }//MVV/LAA

    int attackerPenalty = isProtected ? Tuning::AttackerValue[attacker] :0;
    return Tuning::CaptureBaseScore + (Tuning::VictimValue[victim] * 10) - Tuning::AttackerValue[attacker];
}
}

#endif // TUNING_PARAMS_H