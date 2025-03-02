/*
 * File: z_en_minifrog.c
 * Overlay: ovl_En_Minifrog
 * Description: Five Frogs of the Frog Choir
 */

#include "z_en_minifrog.h"
#include "objects/object_fr/object_fr.h"
#include "objects/gameplay_keep/gameplay_keep.h"

#define FLAGS (ACTOR_FLAG_1 | ACTOR_FLAG_8 | ACTOR_FLAG_10)

#define THIS ((EnMinifrog*)thisx)

void EnMinifrog_Init(Actor* thisx, PlayState* play);
void EnMinifrog_Destroy(Actor* thisx, PlayState* play);
void EnMinifrog_Update(Actor* thisx, PlayState* play);
void EnMinifrog_Draw(Actor* thisx, PlayState* play);

EnMinifrog* EnMinifrog_GetFrog(PlayState* play);

void EnMinifrog_SpawnGrowAndShrink(EnMinifrog* this, PlayState* play);
void EnMinifrog_Idle(EnMinifrog* this, PlayState* play);
void EnMinifrog_SetupNextFrogInit(EnMinifrog* this, PlayState* play);
void EnMinifrog_UpdateMissingFrog(Actor* thisx, PlayState* play);
void EnMinifrog_YellowFrogDialog(EnMinifrog* this, PlayState* play);
void EnMinifrog_SetupYellowFrogDialog(EnMinifrog* this, PlayState* play);

ActorInit En_Minifrog_InitVars = {
    ACTOR_EN_MINIFROG,
    ACTORCAT_NPC,
    FLAGS,
    OBJECT_FR,
    sizeof(EnMinifrog),
    (ActorFunc)EnMinifrog_Init,
    (ActorFunc)EnMinifrog_Destroy,
    (ActorFunc)EnMinifrog_Update,
    (ActorFunc)EnMinifrog_Draw,
};

static ColliderCylinderInit sCylinderInit = {
    {
        COLTYPE_NONE,
        AT_NONE,
        AC_NONE,
        OC1_ON | OC1_TYPE_ALL,
        OC2_TYPE_1,
        COLSHAPE_CYLINDER,
    },
    {
        ELEMTYPE_UNK1,
        { 0xF7CFFFFF, 0x00, 0x00 },
        { 0xF7CFFFFF, 0x00, 0x00 },
        TOUCH_NONE | TOUCH_SFX_NORMAL,
        BUMP_NONE,
        OCELEM_ON,
    },
    { 12, 14, 0, { 0, 0, 0 } },
};

static CollisionCheckInfoInit sColChkInfoInit = { 1, 12, 14, MASS_IMMOVABLE };

// sEyeTextures???
static TexturePtr D_808A4D74[] = {
    object_fr_Tex_0059A0,
    object_fr_Tex_005BA0,
};

static u16 isFrogReturnedFlags[] = {
    0, WEEKEVENTREG_32_40, WEEKEVENTREG_32_80, WEEKEVENTREG_33_01, WEEKEVENTREG_33_02,
};

static s32 isInitialized = false;

static InitChainEntry sInitChain[] = {
    ICHAIN_F32_DIV1000(gravity, -800, ICHAIN_STOP),
};

void EnMinifrog_Init(Actor* thisx, PlayState* play) {
    EnMinifrog* this = THIS;
    int i;

    Actor_ProcessInitChain(&this->actor, sInitChain);
    ActorShape_Init(&this->actor.shape, 0.0f, ActorShadow_DrawCircle, 15.0f);
    SkelAnime_InitFlex(play, &this->skelAnime, &object_fr_Skel_00B538, &object_fr_Anim_001534, this->jointTable,
                       this->morphTable, 24);
    CollisionCheck_SetInfo(&this->actor.colChkInfo, NULL, &sColChkInfoInit);
    Collider_InitAndSetCylinder(play, &this->collider, &this->actor, &sCylinderInit);

    if (!isInitialized) {
        for (i = 0; i < ARRAY_COUNT(D_808A4D74); i++) {
            D_808A4D74[i] = Lib_SegmentedToVirtual(D_808A4D74[i]);
        }
        isInitialized = true;
    }

    this->frogIndex = (this->actor.params & 0xF);
    if (this->frogIndex >= 5) {
        this->frogIndex = MINIFROG_YELLOW;
    }

    this->actor.speed = 0.0f;
    this->actionFunc = EnMinifrog_Idle;
    this->jumpState = MINIFROG_STATE_GROUND;
    this->flags = 0;
    this->timer = 0;

    if (1) {}

    if (!EN_MINIFROG_IS_RETURNED(&this->actor)) {
        if ((this->frogIndex == MINIFROG_YELLOW) || CHECK_WEEKEVENTREG(isFrogReturnedFlags[this->frogIndex])) {
            Actor_Kill(&this->actor);
            return;
        }

        this->timer = 30;
        this->actionFunc = EnMinifrog_SpawnGrowAndShrink;
        this->actor.textId = 0xD81;
        this->actor.colChkInfo.mass = 30;
    } else { // Frogs in mountain village
        if (this->frogIndex == MINIFROG_YELLOW) {
            this->actor.textId = 0;
            this->actionFunc = EnMinifrog_SetupYellowFrogDialog;

            if (!CHECK_WEEKEVENTREG(WEEKEVENTREG_34_01)) {
                this->actor.flags |= ACTOR_FLAG_10000;
            }

            this->actor.home.rot.x = this->actor.home.rot.z = 0;
            this->frog = NULL;
        } else {
            this->frog = EnMinifrog_GetFrog(play);
            this->actor.flags &= ~ACTOR_FLAG_1;

            // Frog has been returned
            if (CHECK_WEEKEVENTREG(isFrogReturnedFlags[this->frogIndex])) {
                this->actionFunc = EnMinifrog_SetupNextFrogInit;
            } else {
                this->actor.draw = NULL;
                this->actor.update = EnMinifrog_UpdateMissingFrog;
            }
        }
    }
}

void EnMinifrog_Destroy(Actor* thisx, PlayState* play) {
    EnMinifrog* this = THIS;

    Collider_DestroyCylinder(play, &this->collider);
    if (this->flags & 0x100) {
        func_801A1F88();
    }
}

EnMinifrog* EnMinifrog_GetFrog(PlayState* play) {
    EnMinifrog* frog = (EnMinifrog*)play->actorCtx.actorLists[ACTORCAT_NPC].first;

    while (frog != NULL) {
        if ((frog->actor.id != ACTOR_EN_MINIFROG) || (frog->actor.params & 0xF)) {
            frog = (EnMinifrog*)frog->actor.next;
        } else {
            return frog;
        }
    }

    return NULL;
}

void EnMinifrog_SetJumpState(EnMinifrog* this) {
    if (this->jumpState == MINIFROG_STATE_GROUND) {
        this->jumpState = MINIFROG_STATE_JUMP;
        Animation_Change(&this->skelAnime, &object_fr_Anim_0007BC, 1.0f, 0.0f, 7.0f, ANIMMODE_ONCE, -5.0f);
    }
}

void EnMinifrog_JumpTimer(EnMinifrog* this) {
    if (this->timer > 0) {
        this->timer--;
    } else {
        this->timer = 60 + (s32)Rand_ZeroFloat(40.0f);
        EnMinifrog_SetJumpState(this);
    }
}

void EnMinifrog_Jump(EnMinifrog* this) {
    SkelAnime_Update(&this->skelAnime);
    switch (this->jumpState) {
        case MINIFROG_STATE_JUMP:
            if (Animation_OnFrame(&this->skelAnime, 4.0f)) {
                this->actor.bgCheckFlags &= ~BGCHECKFLAG_GROUND;
                this->actor.velocity.y = 6.0f;
                Actor_PlaySfx(&this->actor, NA_SE_EV_FROG_JUMP);
                this->jumpState = MINIFROG_STATE_AIR;
            }
            break;
        case MINIFROG_STATE_AIR:
            if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) {
                this->jumpState = MINIFROG_STATE_GROUND;
                Animation_MorphToLoop(&this->skelAnime, &object_fr_Anim_001534, -2.5f);
                SkelAnime_Update(&this->skelAnime);
            }
            break;
    }
}

void EnMinifrog_TurnToPlayer(EnMinifrog* this) {
    Math_ScaledStepToS(&this->actor.shape.rot.y, this->actor.yawTowardsPlayer, 0x400);
    this->actor.world.rot.y = this->actor.shape.rot.y;
}

void EnMinifrog_TurnToMissingFrog(EnMinifrog* this) {
    Math_ScaledStepToS(&this->actor.shape.rot.y, this->actor.home.rot.y, 0x400);
    this->actor.world.rot.y = this->actor.shape.rot.y;
}

static Color_RGBA8 sPrimColor = { 255, 255, 255, 255 };
static Color_RGBA8 sEnvColor = { 80, 80, 80, 255 };

void EnMinifrog_SpawnDust(EnMinifrog* this, PlayState* play) {
    Vec3f pos;
    Vec3f vec5;
    Vec3f vel;
    Vec3f accel;
    s16 yaw;
    s16 pitch;
    Vec3f eye = GET_ACTIVE_CAM(play)->eye;
    s32 i;

    yaw = Math_Vec3f_Yaw(&eye, &this->actor.world.pos);
    pitch = -Math_Vec3f_Pitch(&eye, &this->actor.world.pos);
    vec5.x = this->actor.world.pos.x - (5.0f * Math_SinS(yaw) * Math_CosS(pitch));
    vec5.y = this->actor.world.pos.y - (5.0f * Math_SinS(pitch));
    vec5.z = this->actor.world.pos.z - (5.0f * Math_CosS(yaw) * Math_CosS(pitch));

    for (i = 0; i < 5; i++) {
        vel.x = Rand_CenteredFloat(4.0f);
        vel.y = Rand_CenteredFloat(4.0f);
        vel.z = Rand_CenteredFloat(4.0f);
        accel.x = -vel.x * 0.1f;
        accel.y = -vel.y * 0.1f;
        accel.z = -vel.z * 0.1f;
        pos.x = vec5.x + vel.x;
        pos.y = vec5.y + vel.y;
        pos.z = vec5.z + vel.z;
        func_800B0F80(play, &pos, &vel, &accel, &sPrimColor, &sEnvColor, 300, 30, 10);
    }
}

void EnMinifrog_ReturnFrogCutscene(EnMinifrog* this, PlayState* play) {
    EnMinifrog_TurnToPlayer(this);
    EnMinifrog_Jump(this);

    if ((Message_GetState(&play->msgCtx) == TEXT_STATE_5) && Message_ShouldAdvance(play)) {
        EnMinifrog_SetJumpState(this);

        switch (play->msgCtx.currentTextId) {
            case 0xD81:
            case 0xD83:
            case 0xD84:
            case 0xD86:
            case 0xD87:
                Message_ContinueTextbox(play, play->msgCtx.currentTextId + 1);
                break;

            case 0xD82:
                if (CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE)) {
                    Message_ContinueTextbox(play, 0xD83);
                } else {
                    Message_ContinueTextbox(play, 0xD86);
                }

                SET_WEEKEVENTREG(isFrogReturnedFlags[this->frogIndex]);
                break;

            case 0xD85:
            default:
                Message_CloseTextbox(play);
                EnMinifrog_SpawnDust(this, play);
                SoundSource_PlaySfxAtFixedWorldPos(play, &this->actor.world.pos, 30, NA_SE_EN_NPC_FADEAWAY);
                if (this->actor.csId != CS_ID_NONE) {
                    if (CutsceneManager_GetCurrentCsId() == this->actor.csId) {
                        CutsceneManager_Stop(this->actor.csId);
                    }
                }

                Actor_Kill(&this->actor);
                return;
        }
    }

    if (this->flags & 1) {
        if (CutsceneManager_GetCurrentCsId() == CS_ID_GLOBAL_TALK) {
            CutsceneManager_Stop(CS_ID_GLOBAL_TALK);
            CutsceneManager_Queue(this->actor.csId);
        } else if (CutsceneManager_IsNext(this->actor.csId)) {
            CutsceneManager_Start(this->actor.csId, &this->actor);
            this->flags &= ~1;
        } else {
            CutsceneManager_Queue(this->actor.csId);
        }
    }
}

// This can be seen when the Cyan and Pink frogs spawn after their respective minibosses
void EnMinifrog_SpawnGrowAndShrink(EnMinifrog* this, PlayState* play) {
    EnMinifrog_Jump(this);
    if (this->timer > 0) {
        Actor_SetScale(&this->actor, (0.01f + 0.0003f * this->timer * Math_SinS(0x1000 * (this->timer & 7))));
        this->timer--;
    } else {
        Actor_SetScale(&this->actor, 0.01f);
        this->actionFunc = EnMinifrog_Idle;
    }
}

void EnMinifrog_Idle(EnMinifrog* this, PlayState* play) {
    EnMinifrog_TurnToPlayer(this);
    EnMinifrog_Jump(this);
    EnMinifrog_JumpTimer(this);
    if (Actor_ProcessTalkRequest(&this->actor, &play->state)) {
        this->actionFunc = EnMinifrog_ReturnFrogCutscene;
        if (this->actor.csId != CS_ID_NONE) {
            this->flags |= 1;
        }
    } else if ((this->actor.xzDistToPlayer < 100.0f) && Player_IsFacingActor(&this->actor, 0x3000, play) &&
               (Player_GetMask(play) == PLAYER_MASK_DON_GERO)) {
        func_800B8614(&this->actor, play, 110.0f);
    }
}

void EnMinifrog_SetupNextFrogInit(EnMinifrog* this, PlayState* play) {
    EnMinifrog* nextFrog;
    EnMinifrog* missingFrog;

    EnMinifrog_Jump(this);
    nextFrog = this->frog;
    if (nextFrog != NULL) {
        missingFrog = nextFrog->frog;
        if (nextFrog->frog != NULL) {
            this->actor.home.rot.y =
                (s16)Actor_WorldYawTowardActor(&this->actor, &missingFrog->actor); // Set home to missing frog
            EnMinifrog_TurnToMissingFrog(this);
        } else {
            EnMinifrog_TurnToPlayer(this);
        }

        if (this->frog->actor.home.rot.z == (this->actor.params & 0xF)) {
            EnMinifrog_SetJumpState(this);
            this->frog->actor.home.rot.z = 0;
        }
    }
}

void EnMinifrog_CheckChoirSuccess(EnMinifrog* this, PlayState* play) {
    this->actionFunc = EnMinifrog_YellowFrogDialog;
    if (this->frog != NULL) {
        Message_ContinueTextbox(play, 0xD78);
    } else {
        Message_ContinueTextbox(play, 0xD7C);
    }

    EnMinifrog_SetJumpState(this);
    this->frog = NULL;
    this->actor.home.rot.z = 0;
}

void EnMinifrog_ContinueChoirCutscene(EnMinifrog* this, PlayState* play) {
    EnMinifrog_Jump(this);
    if (CutsceneManager_GetCurrentCsId() == CS_ID_GLOBAL_TALK) {
        EnMinifrog_CheckChoirSuccess(this, play);
        return; // necessary to match
    } else if (CutsceneManager_IsNext(CS_ID_GLOBAL_TALK)) {
        CutsceneManager_Start(CS_ID_GLOBAL_TALK, NULL);
        EnMinifrog_CheckChoirSuccess(this, play);
        return; // necessary to match
    } else if ((this->actor.csId != CS_ID_NONE) && (CutsceneManager_GetCurrentCsId() == this->actor.csId)) {
        CutsceneManager_Stop(this->actor.csId);
        CutsceneManager_Queue(CS_ID_GLOBAL_TALK);
        return; // necessary to match
    } else {
        CutsceneManager_Queue(CS_ID_GLOBAL_TALK);
    }
}

void EnMinifrog_NextFrogMissing(EnMinifrog* this, PlayState* play) {
    EnMinifrog_TurnToMissingFrog(this);
    EnMinifrog_Jump(this);
    if (this->timer > 0) {
        this->timer--;
    } else {
        this->actionFunc = EnMinifrog_ContinueChoirCutscene;
    }
}

void EnMinifrog_NextFrogReturned(EnMinifrog* this, PlayState* play) {
    EnMinifrog_Jump(this);
    if (this->timer > 0) {
        this->timer--;
    } else {
        this->actionFunc = EnMinifrog_ContinueChoirCutscene;
        this->flags &= ~(0x2 << MINIFROG_YELLOW | 0x2 << MINIFROG_CYAN | 0x2 << MINIFROG_PINK | 0x2 << MINIFROG_BLUE |
                         0x2 << MINIFROG_WHITE);
        play->setPlayerTalkAnim(play, &gPlayerAnim_link_normal_talk_free_wait, ANIMMODE_LOOP);
    }
}

void EnMinifrog_SetupNextFrogChoir(EnMinifrog* this, PlayState* play) {
    u8 frogIndex;

    EnMinifrog_Jump(this);
    frogIndex = func_801A39F8();
    if (frogIndex != MINIFROG_INVALID) {
        if (frogIndex == MINIFROG_YELLOW) {
            EnMinifrog_SetJumpState(this);
        } else {
            this->actor.home.rot.z = frogIndex; // This is strange to store the frog index in home z rotation
        }

        if (!(this->flags & (0x2 << frogIndex))) {
            this->flags |= (0x2 << frogIndex);
            this->timer--;
        }
    }

    if (this->frog != NULL) {
        this->actor.home.rot.z = 0;
        this->actionFunc = EnMinifrog_NextFrogMissing;
        this->timer = 60;
        this->actor.home.rot.y = Actor_WorldYawTowardActor(&this->actor, &this->frog->actor);
        func_801A1F88();
        this->flags &= ~0x100;
        this->flags &= ~(0x2 << MINIFROG_YELLOW | 0x2 << MINIFROG_CYAN | 0x2 << MINIFROG_PINK | 0x2 << MINIFROG_BLUE |
                         0x2 << MINIFROG_WHITE);
        play->setPlayerTalkAnim(play, &gPlayerAnim_link_normal_talk_free_wait, ANIMMODE_LOOP);
    } else if (this->timer <= 0) {
        this->actionFunc = EnMinifrog_NextFrogReturned;
        this->timer = 30;
    }
}

void EnMinifrog_BeginChoirCutscene(EnMinifrog* this, PlayState* play) {
    EnMinifrog_Jump(this);
    if (this->actor.csId == CS_ID_NONE) {
        this->actionFunc = EnMinifrog_SetupNextFrogChoir;
    } else if (CutsceneManager_GetCurrentCsId() == CS_ID_GLOBAL_TALK) {
        CutsceneManager_Stop(CS_ID_GLOBAL_TALK);
        CutsceneManager_Queue(this->actor.csId);
    } else if (CutsceneManager_IsNext(this->actor.csId)) {
        CutsceneManager_Start(this->actor.csId, &this->actor);
        this->actionFunc = EnMinifrog_SetupNextFrogChoir;
        this->timer = 5;
        func_801A1F00(3, NA_BGM_FROG_SONG);
        this->flags |= 0x100;
        play->setPlayerTalkAnim(play, &gPlayerAnim_pn_gakkiplay, ANIMMODE_LOOP);
    } else {
        CutsceneManager_Queue(this->actor.csId);
    }
}

void EnMinifrog_EndChoir(EnMinifrog* this, PlayState* play) {
    EnMinifrog_TurnToPlayer(this);
    EnMinifrog_Jump(this);
    if (Actor_ProcessTalkRequest(&this->actor, &play->state)) {
        Message_StartTextbox(play, 0xD7E, &this->actor);
        this->actionFunc = EnMinifrog_YellowFrogDialog;
    } else {
        func_800B8500(&this->actor, play, 1000.0f, 1000.0f, PLAYER_IA_MINUS1);
    }
}

void EnMinifrog_GetFrogHP(EnMinifrog* this, PlayState* play) {
    EnMinifrog_TurnToPlayer(this);
    EnMinifrog_Jump(this);
    if (Actor_HasParent(&this->actor, play)) {
        this->actor.parent = NULL;
        this->actionFunc = EnMinifrog_EndChoir;
        this->actor.flags |= ACTOR_FLAG_10000;
        func_800B8500(&this->actor, play, 1000.0f, 1000.0f, PLAYER_IA_NONE);
    } else {
        Actor_OfferGetItem(&this->actor, play, GI_HEART_PIECE, 10000.0f, 50.0f);
    }
}

void EnMinifrog_YellowFrogDialog(EnMinifrog* this, PlayState* play) {
    EnMinifrog_TurnToPlayer(this);
    EnMinifrog_Jump(this);
    switch (Message_GetState(&play->msgCtx)) {
        case TEXT_STATE_CHOICE:
            if (Message_ShouldAdvance(play)) {
                switch (play->msgCtx.choiceIndex) {
                    case 0: // Yes
                        Audio_PlaySfx_MessageDecide();
                        this->actionFunc = EnMinifrog_BeginChoirCutscene;
                        play->msgCtx.msgLength = 0;
                        break;
                    case 1: // No
                        Audio_PlaySfx_MessageCancel();
                        Message_ContinueTextbox(play, 0xD7E);
                        break;
                }
            }
            break;

        case TEXT_STATE_5:
            if (Message_ShouldAdvance(play)) {
                EnMinifrog_SetJumpState(this);
                switch (play->msgCtx.currentTextId) {
                    case 0xD76:
                        Message_ContinueTextbox(play, play->msgCtx.currentTextId + 1);
                        this->actor.flags &= ~ACTOR_FLAG_10000;
                        SET_WEEKEVENTREG(WEEKEVENTREG_34_01);
                        break;
                    case 0xD78:
                    case 0xD79:
                    case 0xD7A:
                    case 0xD7F:
                        Message_ContinueTextbox(play, play->msgCtx.currentTextId + 1);
                        break;
                    case 0xD77:
                        this->actionFunc = EnMinifrog_BeginChoirCutscene;
                        play->msgCtx.msgLength = 0;
                        break;
                    case 0xD7C:
                        if (CHECK_WEEKEVENTREG(WEEKEVENTREG_35_80)) { // Obtained Heart Piece
                            Message_ContinueTextbox(play, 0xD7E);
                        } else {
                            Message_ContinueTextbox(play, 0xD7D); // Get Heart Piece
                            SET_WEEKEVENTREG(WEEKEVENTREG_35_80);
                        }
                        break;
                    case 0xD7D:
                        Message_CloseTextbox(play);
                        this->actionFunc = EnMinifrog_GetFrogHP;
                        EnMinifrog_GetFrogHP(this, play);
                        break;
                    case 0xD7B:
                    case 0xD7E:
                    default:
                        Message_CloseTextbox(play);
                        this->actionFunc = EnMinifrog_SetupYellowFrogDialog;
                        this->actor.flags &= ~ACTOR_FLAG_10000;
                        break;
                }
            }
            break;
    }
}

void EnMinifrog_SetupYellowFrogDialog(EnMinifrog* this, PlayState* play) {
    Math_ScaledStepToS(&this->actor.shape.rot.y, this->actor.yawTowardsPlayer, 0x180);
    this->actor.world.rot.y = this->actor.shape.rot.y;
    EnMinifrog_TurnToPlayer(this);
    EnMinifrog_Jump(this);
    EnMinifrog_JumpTimer(this);
    if (Actor_ProcessTalkRequest(&this->actor, &play->state)) {
        this->actionFunc = EnMinifrog_YellowFrogDialog;
        if (!CHECK_WEEKEVENTREG(WEEKEVENTREG_34_01)) {
            Message_StartTextbox(play, 0xD76, &this->actor);
        } else {
            Message_StartTextbox(play, 0xD7F, &this->actor);
        }
    } else if ((this->actor.xzDistToPlayer < 150.0f) &&
               (Player_IsFacingActor(&this->actor, 0x3000, play) ||
                CHECK_FLAG_ALL(this->actor.flags, ACTOR_FLAG_10000)) &&
               Player_GetMask(play) == PLAYER_MASK_DON_GERO) {
        func_800B8614(&this->actor, play, 160.0f);
    }
}

void EnMinifrog_Update(Actor* thisx, PlayState* play) {
    EnMinifrog* this = THIS;
    s32 pad;

    this->actionFunc(this, play);
    Actor_MoveWithGravity(&this->actor);
    Actor_UpdateBgCheckInfo(play, &this->actor, 25.0f, 12.0f, 0.0f,
                            UPDBGCHECKINFO_FLAG_1 | UPDBGCHECKINFO_FLAG_4 | UPDBGCHECKINFO_FLAG_8 |
                                UPDBGCHECKINFO_FLAG_10);
    Collider_UpdateCylinder(&this->actor, &this->collider);
    CollisionCheck_SetOC(play, &play->colChkCtx, &this->collider.base);
    this->actor.focus.rot.y = this->actor.shape.rot.y;
}

void EnMinifrog_UpdateMissingFrog(Actor* thisx, PlayState* play) {
    EnMinifrog* this = THIS;
    EnMinifrog* missingFrog;

    missingFrog = this->frog;
    if ((missingFrog != NULL) && (missingFrog->actor.home.rot.z == (this->actor.params & 0xF))) {
        missingFrog->frog = this;
    }
}

s32 EnMinifrog_OverrideLimbDraw(PlayState* play, s32 limbIndex, Gfx** dList, Vec3f* pos, Vec3s* rot, Actor* thisx) {
    if (limbIndex == 1) {
        pos->z -= 500.0f;
    }

    if ((limbIndex == 7) || (limbIndex == 8)) {
        *dList = NULL;
    }

    return false;
}

void EnMinifrog_PostLimbDraw(PlayState* play, s32 limbIndex, Gfx** dList, Vec3s* rot, Actor* thisx) {
    EnMinifrog* this = THIS;

    if ((limbIndex == 7) || (limbIndex == 8)) {
        OPEN_DISPS(play->state.gfxCtx);

        Matrix_ReplaceRotation(&play->billboardMtxF);
        gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gSPDisplayList(POLY_OPA_DISP++, *dList);

        CLOSE_DISPS(play->state.gfxCtx);
    }

    if (limbIndex == 4) {
        Matrix_MultZero(&this->actor.focus.pos);
    }
}

static Color_RGBA8 sEnMinifrogColor[] = {
    { 200, 170, 0, 255 }, { 0, 170, 200, 255 }, { 210, 120, 100, 255 }, { 120, 130, 230, 255 }, { 190, 190, 190, 255 },
};

void EnMinifrog_Draw(Actor* thisx, PlayState* play) {
    EnMinifrog* this = THIS;
    Color_RGBA8* envColor;

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL25_Opa(play->state.gfxCtx);
    envColor = &sEnMinifrogColor[this->frogIndex];
    gSPSegment(POLY_OPA_DISP++, 0x08, D_808A4D74[0]);
    gSPSegment(POLY_OPA_DISP++, 0x09, D_808A4D74[0]);
    gDPSetEnvColor(POLY_OPA_DISP++, envColor->r, envColor->g, envColor->b, envColor->a);
    SkelAnime_DrawFlexOpa(play, this->skelAnime.skeleton, this->skelAnime.jointTable, this->skelAnime.dListCount,
                          EnMinifrog_OverrideLimbDraw, EnMinifrog_PostLimbDraw, &this->actor);

    CLOSE_DISPS(play->state.gfxCtx);
}
