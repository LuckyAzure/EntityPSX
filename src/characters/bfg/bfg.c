#include "bfg.h"

#include "../../psx/mem.h"
#include "../../psx/archive.h"
#include "../../psx/random.h"
#include "../../scenes/stage/stage.h"
#include "../../main.h"

//Boyfriend skull fragments
static SkullFragment char_bfg_skull[15] = {
	{ 1 * 8, -87 * 8, -13, -13},
	{ 9 * 8, -88 * 8,   5, -22},
	{18 * 8, -87 * 8,   9, -22},
	{26 * 8, -85 * 8,  13, -13},
	
	{-3 * 8, -82 * 8, -13, -11},
	{ 8 * 8, -85 * 8,  -9, -15},
	{20 * 8, -82 * 8,   9, -15},
	{30 * 8, -79 * 8,  13, -11},
	
	{-1 * 8, -74 * 8, -13, -5},
	{ 8 * 8, -77 * 8,  -9, -9},
	{19 * 8, -75 * 8,   9, -9},
	{26 * 8, -74 * 8,  13, -5},
	
	{ 5 * 8, -73 * 8, -5, -3},
	{14 * 8, -76 * 8,  9, -6},
	{26 * 8, -67 * 8, 15, -3},
};

//Boyfriend player types
enum
{
	BFG_ArcMain_BFG0,
	BFG_ArcMain_BFG1,
	BFG_ArcMain_BFG2,
	BFG_ArcMain_BFG3,
	BFG_ArcMain_BFG4,
	BFG_ArcMain_BFG5,
	BFG_ArcMain_BFG6,
	BFG_ArcMain_Dead0, //BREAK
	BFG_ArcDead_Dead1, //Mic Drop
	BFG_ArcDead_Dead2, //Twitch
	BFG_ArcDead_Retry,
	
	BFG_ArcMain_Max,
};

#define BFG_Arc_Max BFG_ArcMain_Max

typedef struct
{
	//Character base structure
	Character character;
	
	//Render data and state
	IO_Data arc_main, arc_dead;
	CdlFILE file_dead_arc; //dead.arc file position
	IO_Data arc_ptr[BFG_Arc_Max];
	
	Gfx_Tex tex, tex_retry;
	u8 frame, tex_id;
	
	u8 retry_bump;
	
	SkullFragment skull[COUNT_OF(char_bfg_skull)];
	u8 skull_scale;
} Char_BFG;

//Boyfriend player definitions
static const CharFrame char_bfg_frame[] = {
	{BFG_ArcMain_BFG0, {  0,   0, 102,  99}, { 53,  92}}, //0 idle 1
	{BFG_ArcMain_BFG0, {103,   0, 102,  99}, { 53,  92}}, //1 idle 2
	{BFG_ArcMain_BFG0, {  0, 100, 102, 101}, { 53,  94}}, //2 idle 3
	{BFG_ArcMain_BFG0, {103, 100, 103, 104}, { 53,  97}}, //3 idle 4
	{BFG_ArcMain_BFG1, {  0,   0, 103, 104}, { 53,  97}}, //4 idle 5
	
	{BFG_ArcMain_BFG1, {104,   0,  96, 102}, { 56,  95}}, //5 left 1
	{BFG_ArcMain_BFG1, {  0, 105,  94, 102}, { 54,  95}}, //6 left 2
	
	{BFG_ArcMain_BFG1, { 95, 103,  94,  89}, { 52,  82}}, //7 down 1
	{BFG_ArcMain_BFG2, {  0,   0,  94,  90}, { 52,  83}}, //8 down 2
	
	{BFG_ArcMain_BFG2, { 95,   0,  93, 112}, { 41, 104}}, //9 up 1
	{BFG_ArcMain_BFG2, {  0,  91,  94, 111}, { 42, 103}}, //10 up 2
	
	{BFG_ArcMain_BFG2, { 95, 113, 102, 102}, { 41,  95}}, //11 right 1
	{BFG_ArcMain_BFG3, {  0,   0, 102, 102}, { 41,  95}}, //12 right 2
	
	{BFG_ArcMain_BFG3, {103,   0,  99, 105}, { 54,  98}}, //13 peace 1
	{BFG_ArcMain_BFG3, {  0, 103, 104, 103}, { 54,  96}}, //14 peace 2
	{BFG_ArcMain_BFG3, {105, 106, 104, 104}, { 54,  97}}, //15 peace 3
	
	{BFG_ArcMain_BFG4, {  0,   0, 128, 128}, { 53,  92}}, //16 sweat 1
	{BFG_ArcMain_BFG4, {128,   0, 128, 128}, { 53,  93}}, //17 sweat 2
	{BFG_ArcMain_BFG4, {  0, 128, 128, 128}, { 53,  98}}, //18 sweat 3
	{BFG_ArcMain_BFG4, {128, 128, 128, 128}, { 53,  98}}, //19 sweat 4
	
	{BFG_ArcMain_BFG5, {  0,   0,  93, 108}, { 52, 101}}, //20 left miss 1
	{BFG_ArcMain_BFG5, { 94,   0,  93, 108}, { 52, 101}}, //21 left miss 2
	
	{BFG_ArcMain_BFG5, {  0, 109,  95,  98}, { 50,  90}}, //22 down miss 1
	{BFG_ArcMain_BFG5, { 96, 109,  95,  97}, { 50,  89}}, //23 down miss 2
	
	{BFG_ArcMain_BFG6, {  0,   0,  90, 107}, { 44,  99}}, //24 up miss 1
	{BFG_ArcMain_BFG6, { 91,   0,  89, 108}, { 44, 100}}, //25 up miss 2
	
	{BFG_ArcMain_BFG6, {  0, 108,  99, 108}, { 42, 101}}, //26 right miss 1
	{BFG_ArcMain_BFG6, {100, 109, 101, 108}, { 43, 101}}, //27 right miss 2

	{BFG_ArcMain_Dead0, {  0,   0, 128, 128}, { 53,  98}}, //23 dead0 0
	{BFG_ArcMain_Dead0, {128,   0, 128, 128}, { 53,  98}}, //24 dead0 1
	{BFG_ArcMain_Dead0, {  0, 128, 128, 128}, { 53,  98}}, //25 dead0 2
	{BFG_ArcMain_Dead0, {128, 128, 128, 128}, { 53,  98}}, //26 dead0 3
	
	{BFG_ArcDead_Dead1, {  0,   0, 128, 128}, { 53,  98}}, //27 dead1 0
	{BFG_ArcDead_Dead1, {128,   0, 128, 128}, { 53,  98}}, //28 dead1 1
	{BFG_ArcDead_Dead1, {  0, 128, 128, 128}, { 53,  98}}, //29 dead1 2
	{BFG_ArcDead_Dead1, {128, 128, 128, 128}, { 53,  98}}, //30 dead1 3
	
	{BFG_ArcDead_Dead2, {  0,   0, 128, 128}, { 53,  98}}, //31 dead2 body twitch 0
	{BFG_ArcDead_Dead2, {128,   0, 128, 128}, { 53,  98}}, //32 dead2 body twitch 1
	{BFG_ArcDead_Dead2, {  0, 128, 128, 128}, { 53,  98}}, //33 dead2 balls twitch 0
	{BFG_ArcDead_Dead2, {128, 128, 128, 128}, { 53,  98}}, //34 dead2 balls twitch 1
};

static const Animation char_bfg_anim[PlayerAnim_Max] = {
	{2, (const u8[]){ 0,  1,  2,  3,  4, ASCR_BACK, 1}}, //CharAnim_Idle
	{2, (const u8[]){ 5,  6, ASCR_BACK, 1}},             //CharAnim_Left
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       //CharAnim_LeftAlt
	{2, (const u8[]){ 7,  8, ASCR_BACK, 1}},             //CharAnim_Down
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       //CharAnim_DownAlt
	{2, (const u8[]){ 9, 10, ASCR_BACK, 1}},             //CharAnim_Up
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       //CharAnim_UpAlt
	{2, (const u8[]){11, 12, ASCR_BACK, 1}},             //CharAnim_Right
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       //CharAnim_RightAlt
	
	{1, (const u8[]){ 20, 20, 21, ASCR_BACK, 1}},     //PlayerAnim_LeftMiss
	{1, (const u8[]){ 22, 22, 23, ASCR_BACK, 1}},     //PlayerAnim_DownMiss
	{1, (const u8[]){ 24, 24, 25, ASCR_BACK, 1}},     //PlayerAnim_UpMiss
	{1, (const u8[]){ 26, 26, 27, ASCR_BACK, 1}},     //PlayerAnim_RightMiss
	
	{2, (const u8[]){13, 14, 15, ASCR_BACK, 1}},         //PlayerAnim_Peace
	{2, (const u8[]){16, 17, 18, 19, ASCR_REPEAT}},      //PlayerAnim_Sweat
	
	{2, (const u8[]){28, 29, 30, 31, 31, 31, 31, 31, 31, 31, ASCR_CHGANI, PlayerAnim_Dead1}}, //PlayerAnim_Dead0
	{2, (const u8[]){31, ASCR_REPEAT}},                                                       //PlayerAnim_Dead1
	{3, (const u8[]){32, 33, 34, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, ASCR_CHGANI, PlayerAnim_Dead3}}, //PlayerAnim_Dead2
	{2, (const u8[]){35, ASCR_REPEAT}},                                                       //PlayerAnim_Dead3
	{3, (const u8[]){36, 37, 35, 35, 35, 35, 35, ASCR_CHGANI, PlayerAnim_Dead3}},             //PlayerAnim_Dead4
	{3, (const u8[]){38, 39, 35, 35, 35, 35, 35, ASCR_CHGANI, PlayerAnim_Dead3}},             //PlayerAnim_Dead5
	
	{10, (const u8[]){35, 35, 35, ASCR_BACK, 1}}, //PlayerAnim_Dead4
	{ 3, (const u8[]){38, 39, 35, ASCR_REPEAT}},  //PlayerAnim_Dead5
};

//Boyfriend player functions
void Char_BFG_SetFrame(void *user, u8 frame)
{
	Char_BFG *this = (Char_BFG*)user;
	
	//Check if this is a new frame
	if (frame != this->frame)
	{
		//Check if new art shall be loaded
		const CharFrame *cframe = &char_bfg_frame[this->frame = frame];
		if (cframe->tex != this->tex_id)
			Gfx_LoadTex(&this->tex, this->arc_ptr[this->tex_id = cframe->tex], 0);
	}
}

void Char_BFG_Tick(Character *character)
{
	Char_BFG *this = (Char_BFG*)character;
	
	//Handle animation updates
	if ((character->pad_held & (stage.prefs.control_keys[0] | stage.prefs.control_keys[1] | stage.prefs.control_keys[2] | stage.prefs.control_keys[3])) == 0 ||
	    (character->animatable.anim != CharAnim_Left &&
	     character->animatable.anim != CharAnim_LeftAlt &&
	     character->animatable.anim != CharAnim_Down &&
	     character->animatable.anim != CharAnim_DownAlt &&
	     character->animatable.anim != CharAnim_Up &&
	     character->animatable.anim != CharAnim_UpAlt &&
	     character->animatable.anim != CharAnim_Right &&
	     character->animatable.anim != CharAnim_RightAlt))
		Character_CheckEndSing(character);
	
	if (stage.flag & STAGE_FLAG_JUST_STEP)
	{
		//Perform idle dance
		if (Animatable_Ended(&character->animatable) &&
			(character->animatable.anim != CharAnim_Left &&
		     character->animatable.anim != CharAnim_LeftAlt &&
		     character->animatable.anim != PlayerAnim_LeftMiss &&
		     character->animatable.anim != CharAnim_Down &&
		     character->animatable.anim != CharAnim_DownAlt &&
		     character->animatable.anim != PlayerAnim_DownMiss &&
		     character->animatable.anim != CharAnim_Up &&
		     character->animatable.anim != CharAnim_UpAlt &&
		     character->animatable.anim != PlayerAnim_UpMiss &&
		     character->animatable.anim != CharAnim_Right &&
		     character->animatable.anim != CharAnim_RightAlt &&
		     character->animatable.anim != PlayerAnim_RightMiss) &&
			(stage.song_step & 0x7) == 0)
			character->set_anim(character, CharAnim_Idle);
	}
	
	//Retry screen
	if (character->animatable.anim >= PlayerAnim_Dead2)
		Stage_BlendTex(&this->tex_retry, &stage.retry_src, &stage.retry_dst, FIXED_MUL(stage.camera.zoom, stage.bump), (stage.retry_visibility == 0) ? 0 : 1);
	
	//Animate and draw character
	Animatable_Animate(&character->animatable, (void*)this, Char_BFG_SetFrame);
	Character_Draw(character, &this->tex, &char_bfg_frame[this->frame]);
}

void Char_BFG_SetAnim(Character *character, u8 anim)
{
	Char_BFG *this = (Char_BFG*)character;
	
	//Perform animation checks
	switch (anim)
	{
		case PlayerAnim_Dead0:
			character->focus_x = FIXED_DEC(0,1);
			character->focus_y = FIXED_DEC(-40,1);
			character->focus_zoom = FIXED_DEC(125,100);
			break;
		case PlayerAnim_Dead2:
			//Load retry art
			Gfx_LoadTex(&this->tex_retry, this->arc_ptr[BFG_ArcDead_Retry], 0);
			break;
	}
	
	//Set animation
	Animatable_SetAnim(&character->animatable, anim);
	Character_CheckStartSing(character);
}

void Char_BFG_Free(Character *character)
{
	Char_BFG *this = (Char_BFG*)character;
	
	//Free art
	Mem_Free(this->arc_main);
}

Character *Char_BFG_New(fixed_t x, fixed_t y)
{
	//Allocate boyfriend object
	Char_BFG *this = Mem_Alloc(sizeof(Char_BFG));
	if (this == NULL)
	{
		sprintf(error_msg, "[Char_BFG_New] Failed to allocate boyfriend object");
		ErrorLock();
		return NULL;
	}
	
	//Initialize character
	this->character.tick = Char_BFG_Tick;
	this->character.set_anim = Char_BFG_SetAnim;
	this->character.free = Char_BFG_Free;
	
	Animatable_Init(&this->character.animatable, char_bfg_anim);
	Character_Init((Character*)this, x, y);
	
	//Set character information
	this->character.spec = CHAR_SPEC_MISSANIM;
	
	this->character.health_i[0][0] = 0;
	this->character.health_i[0][1] = 102;
	this->character.health_i[0][2] = 42;
	this->character.health_i[0][3] = 28;
	
	this->character.health_i[1][0] = 43;
	this->character.health_i[1][1] = 102;
	this->character.health_i[1][2] = 42;
	this->character.health_i[1][3] = 34;
	
	this->character.health_i[2][0] = 86;
	this->character.health_i[2][1] = 102;
	this->character.health_i[2][2] = 43;
	this->character.health_i[2][3] = 27;
	
	//health bar color
	this->character.health_bar = 0xFF00A6E2;
	
	this->character.focus_x = FIXED_DEC(-30,1);
	this->character.focus_y = FIXED_DEC(-95,1);
	this->character.focus_zoom = FIXED_DEC(1,1);
	
	this->character.size = FIXED_DEC(1,1);
	
	//Load art
	this->arc_main = IO_Read("\\CHAR\\BFG.ARC;1");
	this->arc_dead = NULL;
	//IO_FindFile(&this->file_dead_arc, "\\CHAR\\BFGDEAD.ARC;1");
	
	const char **pathp = (const char *[]){
		"bf0.tim",   //BFG_ArcMain_BFG0
		"bf1.tim",   //BFG_ArcMain_BFG1
		"bf2.tim",   //BFG_ArcMain_BFG2
		"bf3.tim",   //BFG_ArcMain_BFG3
		"bf4.tim",   //BFG_ArcMain_BFG4
		"bf5.tim",   //BFG_ArcMain_BFG5
		"bf6.tim",   //BFG_ArcMain_BFG6
		"dead0.tim", //BFG_ArcMain_Dead0
		"dead1.tim", //BFG_ArcDead_Dead1
		"dead2.tim", //BFG_ArcDead_Dead2
		"retry.tim", //BFG_ArcDead_Retry
		NULL
	};
	IO_Data *arc_ptr = this->arc_ptr;
	for (; *pathp != NULL; pathp++)
		*arc_ptr++ = Archive_Find(this->arc_main, *pathp);
	
	//Initialize render state
	this->tex_id = this->frame = 0xFF;
	
	//Initialize player state
	this->retry_bump = 0;
	
	//Copy skull fragments
	memcpy(this->skull, char_bfg_skull, sizeof(char_bfg_skull));
	this->skull_scale = 64;
	
	SkullFragment *frag = this->skull;
	for (size_t i = 0; i < COUNT_OF_MEMBER(Char_BFG, skull); i++, frag++)
	{
		//Randomize trajectory
		frag->xsp += RandomRange(-4, 4);
		frag->ysp += RandomRange(-2, 2);
	}
	
	return (Character*)this;
}
