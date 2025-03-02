/*
 * File: z_en_hata.c
 * Overlay: ovl_En_Hata
 * Description: Red Flag on Post
 */

#include "z_en_hata.h"
#include "objects/object_hata/object_hata.h"

#define FLAGS 0x00000000

#define THIS ((EnHata*)thisx)

void EnHata_Init(Actor* thisx, PlayState* play);
void EnHata_Destroy(Actor* thisx, PlayState* play);
void EnHata_Update(Actor* thisx, PlayState* play2);
void EnHata_Draw(Actor* thisx, PlayState* play);

ActorInit En_Hata_InitVars = {
    ACTOR_EN_HATA,
    ACTORCAT_PROP,
    FLAGS,
    OBJECT_HATA,
    sizeof(EnHata),
    (ActorFunc)EnHata_Init,
    (ActorFunc)EnHata_Destroy,
    (ActorFunc)EnHata_Update,
    (ActorFunc)EnHata_Draw,
};

void EnHata_Init(Actor* thisx, PlayState* play) {
    EnHata* this = THIS;
    s32 rand;
    f32 endFrame;

    SkelAnime_Init(play, &this->skelAnime, &object_hata_Skel_002FD0, NULL, this->jointTable, this->morphTable,
                   OBJECT_HATA_LIMB_MAX);
    endFrame = Animation_GetLastFrame(&object_hata_Anim_000444);
    Animation_Change(&this->skelAnime, &object_hata_Anim_000444, 1.0f, 0.0f, endFrame, ANIMMODE_LOOP, 0.0f);
    rand = Rand_ZeroFloat(endFrame);
    this->skelAnime.curFrame = rand;
    DynaPolyActor_LoadMesh(play, &this->dyna, &object_hata_Colheader_0000C0);
    Actor_SetScale(&this->dyna.actor, 0.013f);
    this->dyna.actor.uncullZoneScale = 500.0f;
    this->dyna.actor.uncullZoneDownward = 500.0f;
    this->dyna.actor.uncullZoneForward = 2200.0f;
}

void EnHata_Destroy(Actor* thisx, PlayState* play) {
    EnHata* this = THIS;

    DynaPoly_DeleteBgActor(play, &play->colCtx.dyna, this->dyna.bgId);
}

void EnHata_Update(Actor* thisx, PlayState* play2) {
    PlayState* play = play2;
    EnHata* this = THIS;
    Vec3f sp34;
    f32 phi_fv0;
    s32 pad;

    phi_fv0 = CLAMP(play->envCtx.windSpeed / 120.0f, 0.0f, 1.0f);
    this->skelAnime.playSpeed = 2.75f * phi_fv0;
    this->skelAnime.playSpeed += 1.0f + Rand_ZeroFloat(1.25f);

    sp34.x = play->envCtx.windDir.x;
    sp34.y = play->envCtx.windDir.y + ((1.0f - phi_fv0) * 240.0f);
    sp34.y = CLAMP(sp34.y, -118.0f, 118.0f);
    sp34.z = play->envCtx.windDir.z;

    phi_fv0 = CLAMP(phi_fv0, 0.1f, 0.4f);
    Math_ApproachF(&this->unk_2A4.x, sp34.x, phi_fv0, 1000.0f);
    Math_ApproachF(&this->unk_2A4.y, sp34.y, phi_fv0, 1000.0f);
    Math_ApproachF(&this->unk_2A4.z, sp34.z, phi_fv0, 1000.0f);

    sp34 = this->unk_2A4;
    this->unk_29C = Math_Vec3f_Pitch(&gZeroVec3f, &sp34);
    this->unk_29C = -this->unk_29C;
    this->unk_2A0 = Math_Vec3f_Yaw(&gZeroVec3f, &sp34);
    this->unk_2A0 += -0x4000;
    SkelAnime_Update(&this->skelAnime);
}

s32 EnHata_OverrideLimbDraw(PlayState* play, s32 limbIndex, Gfx** dList, Vec3f* pos, Vec3s* rot, Actor* thisx) {
    EnHata* this = THIS;

    if ((limbIndex == OBJECT_HATA_LIMB_04) || (limbIndex == OBJECT_HATA_LIMB_0D)) {
        rot->y += this->unk_29C;
        rot->z += this->unk_2A0;
    }
    return false;
}

void EnHata_Draw(Actor* thisx, PlayState* play) {
    EnHata* this = THIS;

    Gfx_SetupDL37_Opa(play->state.gfxCtx);
    SkelAnime_DrawOpa(play, this->skelAnime.skeleton, this->skelAnime.jointTable, EnHata_OverrideLimbDraw, NULL,
                      &this->dyna.actor);
}
