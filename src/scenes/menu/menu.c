#include "menu.h"
#include "options.h"

#include "../../main.h"
#include "../../audio.h"

#include "../../psx/mem.h"
#include "../../psx/timer.h"
#include "../../psx/io.h"
#include "../../psx/gfx.h"
#include "../../psx/pad.h"
#include "../../psx/archive.h"
#include "../../psx/mutil.h"
#include "../../psx/trans.h"
#include "../../psx/loadscr.h"

#include "../../fonts/font.h"

#include "../stage/stage.h"

//Menu string type
#define MENUSTR_CHARS 0x20
typedef char MenuStr[MENUSTR_CHARS + 1];

//Menu state
Menu menu;

//Internal menu functions
char menu_text_buffer[0x100];

static void Load_SFX(char *path, u8 offset)
{
	CdlFILE file;
	
	IO_FindFile(&file, path);
   	u32 *data = IO_ReadFile(&file);
    menu.sounds[offset] = Audio_LoadVAGData(data, file.size);
    Mem_Free(data);
}

static const char *Menu_LowerIf(const char *text, boolean lower)
{
	//Copy text
	char *dstp = menu_text_buffer;
	if (lower)
	{
		for (const char *srcp = text; *srcp != '\0'; srcp++)
		{
			if (*srcp >= 'A' && *srcp <= 'Z')
				*dstp++ = *srcp | 0x20;
			else
				*dstp++ = *srcp;
		}
	}
	else
	{
		for (const char *srcp = text; *srcp != '\0'; srcp++)
		{
			if (*srcp >= 'a' && *srcp <= 'z')
				*dstp++ = *srcp & ~0x20;
			else
				*dstp++ = *srcp;
		}
	}
	
	//Terminate text
	*dstp++ = '\0';
	return menu_text_buffer;
}

static void Menu_DrawBack(boolean flash, s32 scroll, u8 r0, u8 g0, u8 b0, u8 r1, u8 g1, u8 b1)
{
	RECT back_src = {0, 0, 255, 255};
	RECT back_dst = {0, -scroll - SCREEN_WIDEADD2, SCREEN_WIDTH, SCREEN_WIDTH * 4 / 5};
	
	if (flash || (animf_count & 4) == 0)
		Gfx_DrawTexCol(&menu.tex_back, &back_src, &back_dst, r0, g0, b0);
	else
		Gfx_DrawTexCol(&menu.tex_back, &back_src, &back_dst, r1, g1, b1);
}

static void Menu_DifficultySelector(s32 x, s32 y)
{
	//Change difficulty
	if (menu.next_page == menu.page && Trans_Idle())
	{
		if (pad_state.press & PAD_DOWN)
		{
			Audio_PlaySound(menu.sounds[0], 0x3fff);
			if (menu.page_param.stage.diff > StageDiff_Easy)
				menu.page_param.stage.diff--;
			else
				menu.page_param.stage.diff = StageDiff_Hard;
		}
		if (pad_state.press & PAD_UP)
		{
			Audio_PlaySound(menu.sounds[0], 0x3fff);
			if (menu.page_param.stage.diff < StageDiff_Hard)
				menu.page_param.stage.diff++;
			else
				menu.page_param.stage.diff = StageDiff_Easy;
		}
	}
	
	//Draw difficulty
	static const RECT diff_srcs[] = {
		{  0, 96, 53, 18},
		{ 60, 96, 89, 18},
		{152, 96, 58, 18},
	};
	
	const RECT *diff_src = &diff_srcs[menu.page_param.stage.diff];
	Gfx_BlitTex(&menu.tex_story, diff_src, x - (diff_src->w >> 1), y - 9 + ((pad_state.press & (PAD_DOWN | PAD_UP)) != 0));
}

static void Menu_DrawWeek(const char *week, s32 x, s32 y)
{
	//Draw label
	if (week == NULL)
	{
		//Tutorial
		RECT label_src = {0, 0, 112, 32};
		Gfx_BlitTex(&menu.tex_story, &label_src, x, y);
	}
	else
	{
		//Week
		RECT label_src = {0, 32, 80, 32};
		Gfx_BlitTex(&menu.tex_story, &label_src, x, y);
		
		//Number
		x += 80;
		for (; *week != '\0'; week++)
		{
			//Draw number
			u8 i = *week - '0';
			
			RECT num_src = {128 + ((i & 3) << 5), ((i >> 2) << 5), 32, 32};
			Gfx_BlitTex(&menu.tex_story, &num_src, x, y);
			x += 32;
		}
	}
}

//Menu functions
void Menu_Load(MenuPage page)
{
	//Load menu assets
	IO_Data menu_arc = IO_Read("\\MENU\\MENU.ARC;1");
	Gfx_LoadTex(&menu.tex_back,  Archive_Find(menu_arc, "back.tim"),  0);
	Gfx_LoadTex(&menu.tex_ng,    Archive_Find(menu_arc, "ng.tim"),    0);
	Gfx_LoadTex(&menu.tex_story, Archive_Find(menu_arc, "story.tim"), 0);
	Gfx_LoadTex(&menu.tex_title, Archive_Find(menu_arc, "title.tim"), 0);
	Gfx_LoadTex(&menu.tex_titleback0, Archive_Find(menu_arc, "titleback0.tim"), 0);
	Gfx_LoadTex(&menu.tex_titleback1, Archive_Find(menu_arc, "titleback1.tim"), 0);
	Gfx_LoadTex(&menu.tex_titleback2, Archive_Find(menu_arc, "titleback2.tim"), 0);
	Gfx_LoadTex(&menu.tex_titleback3, Archive_Find(menu_arc, "titleback3.tim"), 0);
	Gfx_LoadTex(&menu.tex_options, Archive_Find(menu_arc, "options.tim"), 0);
	Gfx_LoadTex(&menu.tex_story0, Archive_Find(menu_arc, "story0.tim"), 0);
	Gfx_LoadTex(&menu.tex_story1, Archive_Find(menu_arc, "story1.tim"), 0);
	Mem_Free(menu_arc);
	
	//Load VAG files
	Audio_ClearAlloc();
	Load_SFX("\\SOUNDS\\SCROLL.VAG;1",0);
	Load_SFX("\\SOUNDS\\CONFIRM.VAG;1",1);
	
	switch (menu.page = menu.next_page = page)
	{
		case MenuPage_Opening:
			//Get funny message to use
			//Do this here so timing is less reliant on VSync
			break;
		default:
			break;
	}
	menu.page_swap = true;
	
	menu.trans_time = 0;
	Trans_Clear();
	
	stage.song_step = 0;
	
	//Play menu music
	Audio_PlayXA_Track(XA_GettinFreaky, 0x40, 0, 1);
	Audio_WaitPlayXA();
	
	//Set background colour
	Gfx_SetClear(0, 0, 0);
}

void Menu_Unload(void)
{
	//Free title Girlfriend
	Character_Free(menu.gf);
}

void Menu_ToStage(StageId id, StageDiff diff, boolean story)
{
	menu.next_page = MenuPage_Stage;
	menu.page_param.stage.id = id;
	menu.page_param.stage.story = story;
	menu.page_param.stage.diff = diff;
	Trans_Start();
}

void Menu_Tick(void)
{
	//Clear per-frame flags
	stage.flag &= ~STAGE_FLAG_JUST_STEP;
	
	//Get song position
	u16 next_step = Audio_TellXA_Milli() / 147; //100 BPM
	if (next_step != stage.song_step)
	{
		if (next_step >= stage.song_step)
			stage.flag |= STAGE_FLAG_JUST_STEP;
		stage.song_step = next_step;
	}
	
	//Handle transition out
	if (Trans_Tick())
	{
		//Change to set next page
		menu.page_swap = true;
		menu.page = menu.next_page;
		menu.select = menu.next_select;
	}
	
	//Tick menu page
	MenuPage exec_page;
	switch (exec_page = menu.page)
	{
		case MenuPage_Opening:
		{
			menu.page = menu.next_page = MenuPage_Title;
			menu.page_swap = true;
		}
	//Fallthrough
		case MenuPage_Title:
		{
			//Initialize page
			if (menu.page_swap)
			{
				menu.page_state.title.logo_bump = (FIXED_DEC(7,1) / 24) - 1;
				menu.page_state.title.fade = FIXED_DEC(255,1);
				menu.page_state.title.fadespd = FIXED_DEC(90,1);
			}
			
			//Draw white fade
			if (menu.page_state.title.fade > 0)
			{
				static const RECT flash = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
				u8 flash_col = menu.page_state.title.fade >> FIXED_SHIFT;
				Gfx_BlendRect(&flash, flash_col, flash_col, flash_col, 1);
				menu.page_state.title.fade -= FIXED_MUL(menu.page_state.title.fadespd, timer_dt);
			}
			
			//Go to main menu when start is pressed
			if (menu.trans_time > 0 && (menu.trans_time -= timer_dt) <= 0)
				Trans_Start();
			
			if ((pad_state.press & PAD_START) && menu.next_page == menu.page && Trans_Idle())
			{
				menu.trans_time = FIXED_UNIT;
				menu.page_state.title.fade = FIXED_DEC(255,1);
				menu.page_state.title.fadespd = FIXED_DEC(300,1);
				menu.next_page = MenuPage_Main;
				menu.next_select = 0;
				Audio_PlaySound(menu.sounds[1], 0x3fff);
			}
			
			//Draw Friday Night Funkin' logo
			RECT logo_src = {0, 0, 212, 166};
			RECT logo_dst = {
				55,
				44,
				logo_src.w,
				logo_src.h
			};
			
			Gfx_DrawTex(&menu.tex_title, &logo_src, &logo_dst);
			
			//Draw "Press Start to Begin"
			RECT press_src = {0, 167, 120, 13};
			Gfx_BlitTex(&menu.tex_title, &press_src, 10, 10);
			
			RECT back0_src = {0, 0, 256, 240};
			Gfx_BlitTex(&menu.tex_titleback0, &back0_src, 0, 0);
			RECT back1_src = {0, 0, 64, 240};
			Gfx_BlitTex(&menu.tex_titleback1, &back1_src, 256, 0);
			break;
		}
		case MenuPage_Main:
		{
			static const char *menu_options[] = {
				"STORY MODE",
				"FREEPLAY",
				"CREDITS",
				"OPTIONS",
			};
			
			//Initialize page
			if (menu.page_swap)
			{
				menu.page_param.stage.diff = StageDiff_Normal;
				menu.scroll = menu.select * FIXED_DEC(12,1);
			}
			
			//Draw version identification
			fonts.font_cdr.draw(&fonts.font_cdr,
				"Plus v0.1 (Alpha)",
				4,
				SCREEN_HEIGHT - 20,
				FontAlign_Left
			);
			
			//Handle option and selection
			if (menu.trans_time > 0 && (menu.trans_time -= timer_dt) <= 0)
				Trans_Start();
			
			if (menu.next_page == menu.page && Trans_Idle())
			{
				//Change option
				if (pad_state.press & PAD_UP)
				{
					Audio_PlaySound(menu.sounds[0], 0x3fff);
					if (menu.select > 0)
						menu.select--;
					else
						menu.select = COUNT_OF(menu_options) - 1;
				}
				if (pad_state.press & PAD_DOWN)
				{
					Audio_PlaySound(menu.sounds[0], 0x3fff);
					if (menu.select < COUNT_OF(menu_options) - 1)
						menu.select++;
					else
						menu.select = 0;
				}
				
				//Select option if cross is pressed
				if (pad_state.press & (PAD_START | PAD_CROSS))
				{
					switch (menu.select)
					{
						case 0: //Story Mode
							menu.next_page = MenuPage_Story;
							break;
						case 1: //Freeplay
							menu.next_page = MenuPage_Freeplay;
							break;
						case 2: //Credits
							menu.next_page = MenuPage_Credits;
							break;
						case 3: //Options
							menu.next_page = MenuPage_Options;
							break;
					}
					Audio_PlaySound(menu.sounds[1], 0x3fff);
					menu.next_select = (menu.next_page == MenuPage_Credits) ? 1 : 0;
					menu.trans_time = FIXED_UNIT;
				}
				
				//Return to title screen if circle is pressed
				if (pad_state.press & PAD_CIRCLE)
				{
					Audio_PlaySound(menu.sounds[0], 0x3fff);
					menu.next_page = MenuPage_Title;
					Trans_Start();
				}
			}
			
			//Draw Friday Night Funkin' logo
			RECT logo_src = {0, 0, 212, 166};
			RECT logo_dst = {
				140,
				10,
				logo_src.w / 1.25,
				logo_src.h / 1.25
			};
			
			Gfx_DrawTex(&menu.tex_title, &logo_src, &logo_dst);
			
			//Draw options
			s32 next_scroll = menu.select * FIXED_DEC(12,1);
			menu.scroll += (next_scroll - menu.scroll) >> 2;
			
			if (menu.next_page == menu.page || menu.next_page == MenuPage_Title)
			{
				//Draw all options
				for (u8 i = 0; i < COUNT_OF(menu_options); i++)
				{
					RECT button_src = {0, 181 + (16 * i), 126, 16};
					RECT button_dst = {
						16,
						128 + (24 * i), 
						button_src.w,
						button_src.h,
					};   
					
					if (i == menu.select)
						Gfx_DrawTex(&menu.tex_title, &button_src, &button_dst);
					else
						Gfx_BlendTex(&menu.tex_title, &button_src, &button_dst, 1);
				}
			}
			else if (animf_count & 2)
			{
				RECT button_src = {0, 181 + (16 * menu.select), 126, 16};
				RECT button_dst = {
					16,
					128 + (24 * menu.select), 
					button_src.w,
					button_src.h,
				};   
				
				Gfx_DrawTex(&menu.tex_title, &button_src, &button_dst);
			}
			
			//Draw background
			RECT back0_src = {0, 0, 256, 240};
			Gfx_BlitTex(&menu.tex_titleback2, &back0_src, 0, 0);
			RECT back1_src = {0, 0, 64, 240};
			Gfx_BlitTex(&menu.tex_titleback3, &back1_src, 256, 0);
			break;
		}
		case MenuPage_Story:
		{
			static const struct
			{
				const char *week;
				StageId stage;
				const char *name;
				const char *tracks[3];
				u32 col;
			} menu_options[] = {
				{NULL, StageId_1_1, "TUTORIAL", {"TUTORIAL", NULL, NULL}, 0xFFFF1D00},
				{NULL, StageId_1_2, "TUTORIAL", {"TUTORIAL", NULL, NULL}, 0xFF808080},
				{NULL, StageId_1_3, "TUTORIAL", {"TUTORIAL", NULL, NULL}, 0xFF00B8FF},
				{NULL, StageId_1_4, "TUTORIAL", {"TUTORIAL", NULL, NULL}, 0xFFFF0981}
			};
			
			//Initialize page
			if (menu.page_swap)
			{
				menu.scroll = 0;
				menu.page_state.title.fade = FIXED_DEC(0,1);
				menu.page_state.title.fadespd = FIXED_DEC(0,1);
			}
			
			//Draw white fade
			if (menu.page_state.title.fade > 0)
			{
				static const RECT flash = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
				u8 flash_col = menu.page_state.title.fade >> FIXED_SHIFT;
				Gfx_BlendRect(&flash, flash_col, flash_col, flash_col, 1);
				menu.page_state.title.fade -= FIXED_MUL(menu.page_state.title.fadespd, timer_dt);
			}
			
			//Draw difficulty selector
			Menu_DifficultySelector(SCREEN_WIDTH2, SCREEN_HEIGHT - 20);
			
			//Handle option and selection
			if (menu.trans_time > 0 && (menu.trans_time -= timer_dt) <= 0)
				Trans_Start();
			
			if (menu.next_page == menu.page && Trans_Idle())
			{
				//Change option
				if (pad_state.press & PAD_LEFT)
				{
					Audio_PlaySound(menu.sounds[0], 0x3fff);
					if (menu.select > 0)
						menu.select--;
					else
						menu.select = COUNT_OF(menu_options) - 1;
				}
				if (pad_state.press & PAD_RIGHT)
				{
					Audio_PlaySound(menu.sounds[0], 0x3fff);
					if (menu.select < COUNT_OF(menu_options) - 1)
						menu.select++;
					else
						menu.select = 0;
				}
				
				//Select option if cross is pressed
				if (pad_state.press & (PAD_START | PAD_CROSS))
				{
					Audio_PlaySound(menu.sounds[1], 0x3fff);
					menu.next_page = MenuPage_Stage;
					menu.page_param.stage.id = menu_options[menu.select].stage;
					menu.page_param.stage.story = true;
					menu.trans_time = FIXED_UNIT;
					menu.page_state.title.fade = FIXED_DEC(255,1);
					menu.page_state.title.fadespd = FIXED_DEC(510,1);
					menu.next_select = menu.select;
				}
				
				//Return to main menu if circle is pressed
				if (pad_state.press & PAD_CIRCLE)
				{
					Audio_PlaySound(menu.sounds[0], 0x3fff);
					menu.next_page = MenuPage_Main;
					menu.next_select = 0; //Story Mode
					Trans_Start();
				}
			}
			
			
			
			RECT entit_src = {72, 33, 128, 36};
			RECT entit_dst = {
				SCREEN_WIDTH2 - 50,
				SCREEN_HEIGHT - 64,
				128 / 1.25,
				36 / 1.25
			};
			
			Gfx_DrawTexCol(&menu.tex_options, &entit_src, &entit_dst,
				((menu_options[menu.select].col >> 16) & 0xFF),
				((menu_options[menu.select].col >>  8) & 0xFF),
				((menu_options[menu.select].col >>  0) & 0xFF)
			);
			
			RECT a_src = {73, 0, 73, 159};
			RECT a_dst = {
				12,
				15,
				73,
				159
			};
			
			Gfx_BlendTex(&menu.tex_story0, &a_src, &a_dst, (menu.select != 0));
			
			RECT b_src = {0, 0, 73, 159};
			RECT b_dst = {
				12 + 75,
				15,
				73,
				159
			};
			
			Gfx_BlendTex(&menu.tex_story0, &b_src, &b_dst, (menu.select != 1));
			
			RECT c_src = {0, 0, 73, 159};
			RECT c_dst = {
				12 + 150,
				15,
				73,
				159
			};
			
			Gfx_BlendTex(&menu.tex_story1, &c_src, &c_dst, (menu.select != 2));
			
			RECT d_src = {73, 0, 73, 159};
			RECT d_dst = {
				12 + 225,
				15,
				73,
				159
			};
			
			Gfx_BlendTex(&menu.tex_story1, &d_src, &d_dst, (menu.select != 3));
			
			break;
		}
		case MenuPage_Freeplay:
		{
			static const struct
			{
				RECT src;
				StageId stage;
				u32 col;
				const char *text;
			} menu_options[] = {
				//{StageId_4_4, 0xFFFC96D7, "TEST"},
				{{140, 0, 33, 36}, StageId_1_1, 0xFF2F2F2F, "SHOTGUN-SHELL"},
				{{0, 28, 39, 30}, StageId_1_2, 0xFFBA1E24, "PARASITE"},
				{{122, 37, 41, 32}, StageId_1_3, 0xFF4CD1E2, "GODRAYS"},
				{{0, 59, 32, 42}, StageId_1_4, 0xFFFFFFFF, "PROMENADE"}
			};
			
			//Initialize page
			if (menu.page_swap)
			{
				menu.scroll = COUNT_OF(menu_options) * FIXED_DEC(24 + SCREEN_HEIGHT2,1);
				menu.page_state.freeplay.back_r = FIXED_DEC(255,1);
				menu.page_state.freeplay.back_g = FIXED_DEC(255,1);
				menu.page_state.freeplay.back_b = FIXED_DEC(255,1);
			}
			
			//Handle option and selection
			if (menu.next_page == menu.page && Trans_Idle())
			{
				//Change option
				if (pad_state.press & PAD_UP)
				{
					Audio_PlaySound(menu.sounds[0], 0x3fff);
					if (menu.select > 0)
						menu.select--;
					else
						menu.select = COUNT_OF(menu_options) - 1;
				}
				if (pad_state.press & PAD_DOWN)
				{
					Audio_PlaySound(menu.sounds[0], 0x3fff);
					if (menu.select < COUNT_OF(menu_options) - 1)
						menu.select++;
					else
						menu.select = 0;
				}
				
				//Select option if cross is pressed
				if (pad_state.press & (PAD_START | PAD_CROSS))
				{
					Audio_PlaySound(menu.sounds[1], 0x3fff);
					menu.next_page = MenuPage_Stage;
					menu.page_param.stage.id = menu_options[menu.select].stage;
					menu.page_param.stage.story = false;
					menu.next_select = menu.select;
					Trans_Start();
				}
				
				//Return to main menu if circle is pressed
				if (pad_state.press & PAD_CIRCLE)
				{
					Audio_PlaySound(menu.sounds[0], 0x3fff);
					menu.next_page = MenuPage_Main;
					menu.next_select = 1; //Freeplay
					Trans_Start();
				}
			}
			
			//Change difficulty
			if (menu.next_page == menu.page && Trans_Idle())
			{
				if (pad_state.press & PAD_LEFT)
				{
					Audio_PlaySound(menu.sounds[0], 0x3fff);
					if (menu.page_param.stage.diff > StageDiff_Easy)
						menu.page_param.stage.diff--;
					else
						menu.page_param.stage.diff = StageDiff_Hard;
				}
				if (pad_state.press & PAD_RIGHT)
				{
					Audio_PlaySound(menu.sounds[0], 0x3fff);
					if (menu.page_param.stage.diff < StageDiff_Hard)
						menu.page_param.stage.diff++;
					else
						menu.page_param.stage.diff = StageDiff_Easy;
				}
			}
			
			//Draw scores and difficulties
			char scoredisp[30];
			sprintf(scoredisp, "PERSONAL BEST: %d", (stage.prefs.savescore[menu_options[menu.select].stage][menu.page_param.stage.diff] > 0) ? stage.prefs.savescore[menu_options[menu.select].stage][menu.page_param.stage.diff] * 10 : 0);
			
			fonts.font_arial.draw(&fonts.font_arial,
				scoredisp,
				SCREEN_WIDTH - 1,
				9,
				FontAlign_Right
			);
			
			static const char *diff[] = {
				"<<EASY>>",
				"<<NORMAL>>",
				"<<HARD>>",
			};
			
			fonts.font_arial.draw(&fonts.font_arial,
				diff[menu.page_param.stage.diff],
				SCREEN_WIDTH - (Font_Arial_GetWidth(&fonts.font_arial,scoredisp) + 3) / 2,
				18,
				FontAlign_Center
			);
			
			RECT desc_back = {
				SCREEN_WIDTH - (Font_Arial_GetWidth(&fonts.font_arial,scoredisp) + 3),
				0,
				SCREEN_WIDTH + Font_Arial_GetWidth(&fonts.font_arial,scoredisp) + 3,
				30
			};
			Gfx_BlendRect(&desc_back, 110, 110, 110, 2);
			
			//Draw options
			
			s32 next_scroll = menu.select * FIXED_DEC(36,1);
			menu.scroll += (next_scroll - menu.scroll) >> 4;
			
			for (u8 i = 0; i < COUNT_OF(menu_options); i++)
			{
				//Get position on screen
				s32 y = (i * 36) - 8 - (menu.scroll >> FIXED_SHIFT);
				if (y <= -SCREEN_HEIGHT2 - 16)
					continue;
				if (y >= SCREEN_HEIGHT2 + 16)
					break;
				
				//Draw icon
				RECT icon_src = menu_options[i].src;
				RECT icon_dst = {
					54 + (y >> 2) - (icon_src.w / 2),
					SCREEN_HEIGHT2 + y - (icon_src.h / 2),
					icon_src.w,
					icon_src.h
				};
				Gfx_DrawTexCol(&stage.tex_icons,
					&icon_src,
					&icon_dst,
					(i == menu.select) ? 128 : 100,
					(i == menu.select) ? 128 : 100,
					(i == menu.select) ? 128 : 100
				);
				
				//Draw text
				fonts.font_bold.draw_col(&fonts.font_bold,
					menu_options[i].text,
					84 + (y >> 2),
					SCREEN_HEIGHT2 + y - 8,
					FontAlign_Left,
					(i == menu.select) ? 128 : 100,
					(i == menu.select) ? 128 : 100,
					(i == menu.select) ? 128 : 100
				);
			}
			
			//Draw background
			fixed_t tgt_r = (fixed_t)((menu_options[menu.select].col >> 16) & 0xFF) << FIXED_SHIFT;
			fixed_t tgt_g = (fixed_t)((menu_options[menu.select].col >>  8) & 0xFF) << FIXED_SHIFT;
			fixed_t tgt_b = (fixed_t)((menu_options[menu.select].col >>  0) & 0xFF) << FIXED_SHIFT;
			
			menu.page_state.freeplay.back_r += (tgt_r - menu.page_state.freeplay.back_r) >> 4;
			menu.page_state.freeplay.back_g += (tgt_g - menu.page_state.freeplay.back_g) >> 4;
			menu.page_state.freeplay.back_b += (tgt_b - menu.page_state.freeplay.back_b) >> 4;
			
			Menu_DrawBack(
				true,
				8,
				menu.page_state.freeplay.back_r >> (FIXED_SHIFT + 1),
				menu.page_state.freeplay.back_g >> (FIXED_SHIFT + 1),
				menu.page_state.freeplay.back_b >> (FIXED_SHIFT + 1),
				0, 0, 0
			);
			break;
		}
		case MenuPage_Credits:
		{
			static const struct
			{
				boolean skip;
				boolean has_icon;
				u8 iconoffset;
				u32 col;
				const char *text;
				const char *desc;
			} credits_options[] = {
				{true, false, NULL, 0xFFFFFFFF, "CREDITS", ""},
				{false, true, 2, 0xFF2CB2E5, "LuckyAzure", "The creator of this port."},
				{true, false, NULL, 0xFFFFFFFF, "ENGINE CREDITS", ""},
				{false, true, 0, 0xFFF6D558, "CuckyDev", "The original creator of PSXFunkin."},
				{false, true, 1, 0xFFB60B00, "spicyjpeg", "He made the save system and sound effects work."},
				{false, true, 2, 0xFF2CB2E5, "LuckyAzure", "The guy who made this modified version of PSXFunkin."},
				{false, true, 3, 0xFFC9AE69, "UNSTOP4BLE", "Helped me a lot with everything.\n(sound effects, save system, some graphic stuff, etc.)"},
				{false, true, 4, 0xFFFD6923, "IgorSou3000", "Also helped with many things too."},
				{false, true, 5, 0xFF0000FF, "BilliousData", "He made the original CDR font."},
				{true, false, NULL, 0xFFFFFFFF, "MOD CREDITS", ""},
				{false, false, NULL, 0xFFFFFFFF, "SugarRatio", "Lead Animator/Artist, Director"},
				{false, false, NULL, 0xFFFFFFFF, "Perez", "Programmer"},
				{false, false, NULL, 0xFFFFFFFF, "The Innuendo", "Primary Composer"},
				{false, false, NULL, 0xFFFFFFFF, "Rozebud", "Featured Composer"},
				{false, false, NULL, 0xFFFFFFFF, "Saruky", "Featured Composer"},
				{false, false, NULL, 0xFFFFFFFF, "CDMusic", "Featured Composer"},
				{false, false, NULL, 0xFFFFFFFF, "Hazelpy", "Programmer"},
				{false, false, NULL, 0xFFFFFFFF, "Tenzubushi", "Animator/Artist"},
				{false, false, NULL, 0xFFFFFFFF, "Moro", "Animator"},
				{false, false, NULL, 0xFFFFFFFF, "Mashprotato", "Animator/Artist"},
				{false, false, NULL, 0xFFFFFFFF, "Ebolahorny", "Animator/Artist"},
				{false, false, NULL, 0xFFFFFFFF, "CheriiDreams", "Animator/Artist"},
				{false, false, NULL, 0xFFFFFFFF, "Netba", "Background Artist"},
				{false, false, NULL, 0xFFFFFFFF, "OrenjiMusic", "Featured Composer"},
				{false, false, NULL, 0xFFFFFFFF, "Elikapelika", "Background Artist"},
				{false, false, NULL, 0xFFFFFFFF, "_liquidmercurie", "Animator/Artist"},
				{false, false, NULL, 0xFFFFFFFF, "Jyro", "Background Artist"},
				{false, false, NULL, 0xFFFFFFFF, "Grass jeak", "Featured Artist"},
				{false, false, NULL, 0xFFFFFFFF, "Brightfyre", "Programmer"},
				{false, false, NULL, 0xFFFFFFFF, "ash", "Programmer"},
				{false, false, NULL, 0xFFFFFFFF, "Hexar", "Programmer"},
				{false, false, NULL, 0xFFFFFFFF, "Mamon", "Vocals for RHO"},
				{false, false, NULL, 0xFFFFFFFF, "MartyKrimson74", "Featured Composer"},
				{false, false, NULL, 0xFFFFFFFF, "Pointy", "Charter"},
				{false, false, NULL, 0xFFFFFFFF, "Cerbera", "Charter"},
				{false, false, NULL, 0xFFFFFFFF, "AlchoholicDJ", "Charter/Animator"},
				{false, false, NULL, 0xFFFFFFFF, "Cougar Mac Dowell", "Voice Actor (Solazar, Agoti)"},
				{false, false, NULL, 0xFFFFFFFF, "JordoPrice", "Voice Actor (Aldryx)"},
				{false, false, NULL, 0xFFFFFFFF, "Katastrofree", "Voice Actor (Nikusa)"},
				{false, false, NULL, 0xFFFFFFFF, "Greyfacered", "Voice Actor (Nikusa Song Voice)"},
				{false, false, NULL, 0xFFFFFFFF, "Alex Vei", "Menu Effect"},
				{false, false, NULL, 0xFFFFFFFF, "iVorart", "Cutscene background for AGOTI, Writer"},
				{false, false, NULL, 0xFFFFFFFF, "pizzapancakess_", "Trailer Editor"},
				{false, false, NULL, 0xFFFFFFFF, "Longest soloever", "Trailer Song"},
			};
			
			//Initialize page
			if (menu.page_swap)
				menu.scroll = COUNT_OF(credits_options) * FIXED_DEC(24 + SCREEN_HEIGHT2,1);
			
			//Handle option and selection
			if (menu.next_page == menu.page && Trans_Idle())
			{
				//Change option
				if (pad_state.press & PAD_UP)
				{
					Audio_PlaySound(menu.sounds[0], 0x3fff);
					if (menu.select > 0 && !credits_options[menu.select - 1].skip)
						menu.select--;
					else if(menu.select > 1)
						menu.select -= 2;
					else
						menu.select = COUNT_OF(credits_options) - 1;
				}
				if (pad_state.press & PAD_DOWN)
				{
					Audio_PlaySound(menu.sounds[0], 0x3fff);
					if ((menu.select < COUNT_OF(credits_options) - 1) && !credits_options[menu.select + 1].skip)
						menu.select++;
					else if((menu.select < COUNT_OF(credits_options) - 2))
						menu.select += 2;
					else
						menu.select = 1;
				}
				
				//Return to main menu if circle is pressed
				if (pad_state.press & PAD_CIRCLE)
				{
					Audio_PlaySound(menu.sounds[0], 0x3fff);
					menu.next_page = MenuPage_Main;
					menu.next_select = 2; //Credits
					Trans_Start();
				}
			}
			
			//Draw desc
			fonts.font_cdr.draw(&fonts.font_cdr,
				credits_options[menu.select].desc,
				32,
				SCREEN_HEIGHT - 42,
				FontAlign_Left
			);
			
			RECT desc_back = {
				0,
				SCREEN_HEIGHT - 48,
				SCREEN_WIDTH,
				32
			};
			Gfx_BlendRect(&desc_back, 110, 110, 110, 2);
			
			//Draw options
			s32 next_scroll = menu.select * FIXED_DEC(30,1);
			menu.scroll += (next_scroll - menu.scroll) >> 4;
			
			for (u8 i = 0; i < COUNT_OF(credits_options); i++)
			{
				//Get position on screen
				s32 y = (i * 30) - 8 - (menu.scroll >> FIXED_SHIFT);
				
				if (y <= -SCREEN_HEIGHT2 - 24)
					continue;
				if (y >= SCREEN_HEIGHT2 + 24)
					break;
				
				if(credits_options[i].has_icon)
				{
					RECT icon_src = {(credits_options[i].iconoffset * 32), 72, 32, 32};
					RECT icon_dst = {
						96 + (y >> 2) + ((i == menu.select) ? 0 : 2),
						SCREEN_HEIGHT2 + y - 14 + ((i == menu.select) ? 0 : 2),
						(i == menu.select) ? 32 : 28,
						(i == menu.select) ? 32 : 28
					};
					Gfx_DrawTexCol(&menu.tex_options,
						&icon_src,
						&icon_dst,
						(i == menu.select) ? 128 : 100,
						(i == menu.select) ? 128 : 100,
						(i == menu.select) ? 128 : 100
					);
				}
				
				//Draw text
				if(!credits_options[i].skip)
					fonts.font_cdr.draw_col(&fonts.font_cdr,
						credits_options[i].text,
						(128 + (y >> 2)) - (credits_options[i].has_icon ? 0 : 32),
						SCREEN_HEIGHT2 + y,
						FontAlign_Left,
						(i == menu.select) ? 128 : 100,
						(i == menu.select) ? 128 : 100,
						(i == menu.select) ? 128 : 100
					);
				else
					fonts.font_bold.draw(&fonts.font_bold,
						credits_options[i].text,
						96 + (y >> 2),
						SCREEN_HEIGHT2 - 8 + y,
						FontAlign_Left
					);
			}
			
			//Draw background
			fixed_t tgt_r = (fixed_t)((credits_options[menu.select].col >> 16) & 0xFF) << FIXED_SHIFT;
			fixed_t tgt_g = (fixed_t)((credits_options[menu.select].col >>  8) & 0xFF) << FIXED_SHIFT;
			fixed_t tgt_b = (fixed_t)((credits_options[menu.select].col >>  0) & 0xFF) << FIXED_SHIFT;
			
			menu.page_state.freeplay.back_r += (tgt_r - menu.page_state.freeplay.back_r) >> 4;
			menu.page_state.freeplay.back_g += (tgt_g - menu.page_state.freeplay.back_g) >> 4;
			menu.page_state.freeplay.back_b += (tgt_b - menu.page_state.freeplay.back_b) >> 4;
			
			Menu_DrawBack(
				true,
				8,
				menu.page_state.freeplay.back_r >> (FIXED_SHIFT + 1),
				menu.page_state.freeplay.back_g >> (FIXED_SHIFT + 1),
				menu.page_state.freeplay.back_b >> (FIXED_SHIFT + 1),
				0, 0, 0
			);
			break;
		}
		case MenuPage_Options:
		{
			Options_Tick();
			
			//Draw background
			Menu_DrawBack(
				true,
				8,
				253 >> 1, 113 >> 1, 155 >> 1,
				0, 0, 0
			);
			break;
		}
		case MenuPage_Stage:
		{
			//Unload menu state
			Menu_Unload();
			
			//Load new stage
			LoadScr_Start();
			Stage_Load(menu.page_param.stage.id, menu.page_param.stage.diff, menu.page_param.stage.story);
			gameloop = GameLoop_Stage;
			LoadScr_End();
			break;
		}
		default:
			break;
	}
	
	//Clear page swap flag
	menu.page_swap = menu.page != exec_page;
}
