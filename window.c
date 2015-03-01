#include "window.h"
#include "lobbyList.h"

pWindow mg_window = NULL;
int window_active = 0;
spSpriteCollectionPointer window_sprite[SPRITE_COUNT];

void init_window_sprites()
{
	int i;
	for (i = 0; i < SPRITE_COUNT; i++)
	{
		char buffer[256];
		sprintf(buffer,"./sprites/hase%i.ssc",i+1);
		window_sprite[i] = spLoadSpriteCollection(buffer,NULL);
		spSelectSprite(window_sprite[i],"high jump right");
		spSetSpriteZoom(spActiveSprite(window_sprite[i]),spGetSizeFactor()/2,spGetSizeFactor()/2);
	}
}

int get_last_sprite()
{
	return window_active+1;
}

void quit_window_sprites()
{
	int i;
	for (i = 0; i < SPRITE_COUNT; i++)
		spDeleteSpriteCollection(window_sprite[i],0);
}	

void update_window_width(pWindow window)
{
	pWindowElement elem = window->firstElement;
	while (elem)
	{
		window->width = spMax(window->width,elem->width+(spGetSizeFactor()*4 >> SP_ACCURACY)*2);
		elem = elem->next;
	}
	window->width = spMin(window->width,spGetWindowSurface()->w-(spGetSizeFactor()*4 >> SP_ACCURACY)*2);
}

pWindow create_window(int ( *feedback )( pWindowElement elem, int action ),spFontPointer font,char* title)
{
	pWindow window = (pWindow)malloc(sizeof(tWindow));
	window->height = font->maxheight*4+(spGetSizeFactor()*4 >> SP_ACCURACY)*2;
	window->font = font;
	window->feedback = feedback;
	window->selection = 0;
	window->firstElement = NULL;
	sprintf(window->title,"%s",title);
	window->width = spMax(spGetWindowSurface()->h,spFontWidth(title,font)+(spGetSizeFactor()*4 >> SP_ACCURACY)*2);
	update_window_width(window);
	window->do_flip = 1;
	window->main_menu = 0;
	window->oldScreen = NULL;
	window->only_ok = 0;
	window->count = 0;
	window->show_selection = 0;
	window->sprite_count = NULL;
	window->insult_button = 0;
	return window;
}

void update_elem_width(pWindowElement elem,pWindow window)
{
	elem->width = spFontWidth(elem->text,window->font);
}

pWindowElement add_window_element(pWindow window,int type,int reference)
{
	pWindowElement elem = (pWindowElement)malloc(sizeof(tWindowElement));
	pWindowElement mom = window->firstElement;
	pWindowElement last = NULL;
	while (mom)
	{
		last = mom;
		mom = mom->next;
	}
	if (last)
		last->next = elem;
	else
		window->firstElement = elem;
	elem->next = NULL;
	elem->type = type;
	elem->reference = reference;
	elem->text[0] = 0;
	window->feedback(elem,WN_ACT_UPDATE);
	update_elem_width(elem,window);
	update_window_width(window);
	window->height+=3*window->font->maxheight/2;
	window->count++;
	return elem;
}

pWindow recent_window = NULL;

void window_draw(void)
{
	SDL_Surface* screen = spGetWindowSurface();
	if (recent_window->oldScreen)
		spBlitSurface(screen->w/2,screen->h/2,0,recent_window->oldScreen);
	spSetPattern8(153,60,102,195,153,60,102,195);
	spRectangle(screen->w/2,screen->h/2,0,screen->w,screen->h,LL_BG);
	spDeactivatePattern();

	if (spIsKeyboardPolled() && spGetVirtualKeyboardState() == SP_VIRTUAL_KEYBOARD_ALWAYS)
		spBlitSurface(screen->w/2,screen->h-spGetVirtualKeyboard()->h/2,0,spGetVirtualKeyboard());

	char buffer[256];
	pWindow window = recent_window;
	int meow = spGetSizeFactor()*4 >> SP_ACCURACY;
	spRectangle(screen->w/2,screen->h/2,0,window->width-2*meow,window->height-2*meow,LL_BG);
	spRectangleBorder(screen->w/2,screen->h/2,0,window->width,window->height,meow,meow,LL_FG);
	int y = (screen->h - window->height) / 2 + meow + window->font->maxheight/2;
	spFontDrawMiddle( screen->w/2, y , 0, window->title, window->font );
	pWindowElement elem = window->firstElement;
	pWindowElement selElem = NULL;
	int nr = 0;
	while (elem)
	{
		y+=window->font->maxheight*3/2;
		if (nr == window->selection)
		{
			if (window->firstElement->next) //more than one element
				spRectangle( screen->w/2, y+window->font->maxheight/2, 0, window->width-2*meow,window->font->maxheight,LL_FG);
			selElem = elem;
			if (elem->type == 1 && spIsKeyboardPolled())
			{
				int x = (screen->w+spFontWidth(elem->text,window->font))/2;
				if ((SDL_GetTicks() >> 9) & 1)
					spLine(x, y, 0, x, y+window->font->maxheight, 0,65535);
			}
		}
		spFontDrawMiddle( screen->w/2, y, 0, elem->text, window->font );
		nr++;
		elem = elem->next;
	}
	
	if (window->show_selection)
	{
		y+=(spGetSizeFactor()*8 >> SP_ACCURACY)+window->font->maxheight*3/2;
		int to_left = -spFontWidth("[r]",window->font);
		if (window->sprite_count && window->sprite_count[window_active])
		{
			if (window->sprite_count[window_active] == 1)
				sprintf(buffer,"[r] (already used one time)");
			else
				sprintf(buffer,"[r] (already used %i times)",window->sprite_count[window_active]);
		}
		else
			sprintf(buffer,"[r]");
		to_left += spFontWidth(buffer,window->font);
		to_left /= 2;
		spDrawSprite(screen->w/2-to_left, y, 0, spActiveSprite(window_sprite[window_active]));
		spFontDrawRight( screen->w/2-to_left-(spGetSizeFactor()*12 >> SP_ACCURACY), y-window->font->maxheight/2, 0, "[l]", window->font );
		spFontDraw     ( screen->w/2-to_left+(spGetSizeFactor()*12 >> SP_ACCURACY), y-window->font->maxheight/2, 0, buffer, window->font );
		y+=(spGetSizeFactor()*8 >> SP_ACCURACY);
		sprintf(buffer,"\"%s\"",window_sprite[window_active]->comment);
		spFontDrawMiddle( screen->w/2, y, 0, buffer, window->font);
		sprintf(buffer,"made by %s (%s)",window_sprite[window_active]->author,window_sprite[window_active]->license);
		y += window->font->maxheight;
		spFontDrawMiddle( screen->w/2, y, 0, buffer, window->font);
	}
	
	
	y = (screen->h + window->height) / 2 - meow - 3*window->font->maxheight/2;
	if (selElem)
	{
		switch (selElem->type)
		{
			case 0: case 2:
				if (window->only_ok)
					spFontDrawMiddle( screen->w/2,y, 0, SP_PAD_NAME": Change  [o]Okay", window->font );
				else
					spFontDrawMiddle( screen->w/2,y, 0, SP_PAD_NAME": Change  [o]Okay  [c]Cancel", window->font );
				break;
			case 1:
				if (spGetVirtualKeyboardState() == SP_VIRTUAL_KEYBOARD_ALWAYS)
				{
					if (spIsKeyboardPolled())
					{
						if (window->only_ok)
						{
							if (window->firstElement->next)
								spFontDrawMiddle( screen->w/2,y, 0, "[o]Enter letter  [R]Back", window->font );
							else
								spFontDrawMiddle( screen->w/2,y, 0, "[o]Enter letter  [R]Okay", window->font );
						}
						else
						{
							if (window->firstElement->next)
								spFontDrawMiddle( screen->w/2,y, 0, "[o]Enter letter  [R]/[c]Back", window->font );
							else
							if (window->insult_button)
								spFontDrawMiddle( screen->w/2,y, 0, "[o]Enter letter  [R]Okay  [c]Cancel [3]Insult", window->font );
							else
								spFontDrawMiddle( screen->w/2,y, 0, "[o]Enter letter  [R]Okay  [c]Cancel", window->font );
						}
					}
					else
					if (window->only_ok)
						spFontDrawMiddle( screen->w/2,y, 0, "[R]Change  [o]Okay", window->font );
					else
						spFontDrawMiddle( screen->w/2,y, 0, "[R]Change  [o]Okay  [c]Cancel", window->font );
				}
				else
				if (window->only_ok)
					spFontDrawMiddle( screen->w/2,y, 0, "Keyboard: Change  [o]Okay", window->font );
				else
				if (window->insult_button)
					spFontDrawMiddle( screen->w/2,y, 0, "Keyboard: Change  [o]Okay  [c]Cancel [3]Insult", window->font );
				else
					spFontDrawMiddle( screen->w/2,y, 0, "Keyboard: Change  [o]Okay  [c]Cancel", window->font );
				break;
			case -1:
				if (window->main_menu)
					spFontDrawMiddle( screen->w/2,y, 0, "[o]Select  [c]Exit", window->font );
				else
				if (window->only_ok)
					spFontDrawMiddle( screen->w/2,y, 0, "[o]Select  [c]Back", window->font );
				else
					spFontDrawMiddle( screen->w/2,y, 0, "[o]Okay  [c]Cancel", window->font );
				break;
			case -2:
				spFontDrawMiddle( screen->w/2,y, 0, "[o]Okay", window->font );
				break;
		}
	}
	else
	if (window->do_flip)
	{
		if (window->only_ok)
			spFontDrawMiddle( screen->w/2,y, 0, "[o]Okay", window->font );
		else
			spFontDrawMiddle( screen->w/2,y, 0, "[o]Okay  [c]Cancel", window->font );
	}
	if (window->do_flip)
		spFlip();
}

void fill_with_insult(char* buffer)
{
	switch (rand()%2)
	{
		case 0: sprintf(buffer,"Son of a witch!"); break;
		case 1: sprintf(buffer,"Your mama's gravity made me miss!"); break;
	}
}

int window_calc(Uint32 steps)
{
	pWindow window = recent_window;
	if (window->insult_button && spGetInput()->button[MY_PRACTICE_3] && spIsKeyboardPolled())
	{
		spGetInput()->button[MY_PRACTICE_3] = 0;
		char buffer[128];
		fill_with_insult(buffer);
		snprintf(&(spGetInput()->keyboard.buffer[spGetInput()->keyboard.pos]),spGetInput()->keyboard.len-spGetInput()->keyboard.pos,"%s",buffer);
		spGetInput()->keyboard.pos += strlen(buffer);
		if (spGetInput()->keyboard.pos > spGetInput()->keyboard.len)
			spGetInput()->keyboard.pos = spGetInput()->keyboard.len;
		spGetInput()->keyboard.lastSize = 1;
	}
	
	if (window->show_selection)
	{
		spUpdateSprite(spActiveSprite(window_sprite[window_active]),steps);
		if (spGetInput()->button[MY_BUTTON_L])
		{
			spGetInput()->button[MY_BUTTON_L] = 0;
			window_active = (window_active + SPRITE_COUNT - 1) % SPRITE_COUNT;
		}
		if (spGetInput()->button[MY_BUTTON_R])
		{
			spGetInput()->button[MY_BUTTON_R] = 0;
			window_active = (window_active + 1) % SPRITE_COUNT;
		}
	}
	pWindowElement selElem = window->firstElement;
	pWindowElement befElem = NULL;
	int nr = 0;
	while (selElem)
	{
		if (nr == window->selection)
			break;
		nr++;
		befElem = selElem;
		selElem = selElem->next;
	}
	if (selElem	== NULL)
	{
		if (spGetInput()->button[MY_PRACTICE_OK] || spGetInput()->button[MY_BUTTON_START] || spGetInput()->button[MY_BUTTON_SELECT])
		{
			spGetInput()->button[MY_PRACTICE_OK] = 0;
			spGetInput()->button[MY_BUTTON_SELECT] = 0;
			spGetInput()->button[MY_BUTTON_START] = 0;
			return 1;
		}
		if (window->only_ok == 0 && spGetInput()->button[MY_PRACTICE_CANCEL])
		{
			spGetInput()->button[MY_PRACTICE_CANCEL] = 0;
			return 2;
		}
		return 0;
	}
	if (befElem == NULL)
	{
		befElem = window->firstElement;
		while (befElem->next)
			befElem = befElem->next;
	}
	if (selElem->type != 1 ||
		!spIsKeyboardPolled() ||
		spGetVirtualKeyboardState() == SP_VIRTUAL_KEYBOARD_NEVER)
	{
		if (spGetInput()->axis[1] < 0)
		{
			spGetInput()->axis[1] = 0;
			if (selElem->type == 1 &&
				spIsKeyboardPolled() && spGetVirtualKeyboardState() == SP_VIRTUAL_KEYBOARD_NEVER)
				window->feedback(selElem,WN_ACT_END_POLL);
			window->selection = (window->selection + window->count - 1) % window->count;
			selElem = befElem;
		}
		if (spGetInput()->axis[1] > 0)
		{
			spGetInput()->axis[1] = 0;
			if (selElem->type == 1 &&
				spIsKeyboardPolled() && spGetVirtualKeyboardState() == SP_VIRTUAL_KEYBOARD_NEVER)
				window->feedback(selElem,WN_ACT_END_POLL);
			window->selection = (window->selection + 1) % window->count;
			if (selElem->next)
				selElem = selElem->next;
			else
				selElem = window->firstElement;
		}
	}
	if ((spGetInput()->button[MY_PRACTICE_OK] && (spGetVirtualKeyboardState() == SP_VIRTUAL_KEYBOARD_NEVER || spIsKeyboardPolled() == 0 ))||
		(spGetInput()->button[MY_BUTTON_SELECT] && window->only_ok && selElem->type != -1))
	{
		spGetInput()->button[MY_PRACTICE_OK] = 0;
		if (window->only_ok && selElem->type != -1)
			spGetInput()->button[MY_BUTTON_SELECT] = 0;
		if (selElem->type == 1)
		{
			if (spIsKeyboardPolled())
			{
				window->feedback(selElem,WN_ACT_END_POLL);
				if (window->firstElement->next == NULL)
					return 1;
				if (spGetVirtualKeyboardState() == SP_VIRTUAL_KEYBOARD_NEVER)
					return 1;
			}
			else
				return 1;
		}
		else
			return 1;
	}
	if ((spGetInput()->button[MY_BUTTON_START] && (spGetVirtualKeyboardState() == SP_VIRTUAL_KEYBOARD_NEVER || spIsKeyboardPolled())) ||
		(spGetInput()->button[MY_BUTTON_SELECT] && window->only_ok && selElem->type != -1))
	{
		spGetInput()->button[MY_BUTTON_START] = 0;
		if (window->only_ok && selElem->type != -1)
			spGetInput()->button[MY_BUTTON_SELECT] = 0;
		if (selElem->type == 1)
		{
			if (spIsKeyboardPolled())
			{
				window->feedback(selElem,WN_ACT_END_POLL);
				if (window->firstElement->next == NULL || spGetVirtualKeyboardState() == SP_VIRTUAL_KEYBOARD_NEVER)
					return 1;
			}
			else
				return 1;
		}
		else
			return 1;
	}
	if ((spGetInput()->button[MY_PRACTICE_CANCEL] || spGetInput()->button[MY_BUTTON_SELECT]) &&
		(window->only_ok == 0 || selElem->type == -1))
	{
		spGetInput()->button[MY_PRACTICE_CANCEL] = 0;
		spGetInput()->button[MY_BUTTON_SELECT] = 0;
		switch (selElem->type)
		{
			case 1:
				if (spIsKeyboardPolled() && spGetVirtualKeyboardState() == SP_VIRTUAL_KEYBOARD_ALWAYS)
				{
					window->feedback(selElem,WN_ACT_END_POLL);
					if (window->firstElement->next == NULL)
						return 2;
				}
				else
					return 2;
				break;
			default:
				return 2;
		}
	}

	if (selElem->type == 1 &&
		!spIsKeyboardPolled() &&
		( spGetVirtualKeyboardState() == SP_VIRTUAL_KEYBOARD_NEVER ||
		  window->firstElement->next == NULL ) )
		window->feedback(selElem,WN_ACT_START_POLL);

	if (spGetInput()->button[MY_BUTTON_START] &&
		selElem->type == 1 &&
		!spIsKeyboardPolled() &&
		spGetVirtualKeyboardState() == SP_VIRTUAL_KEYBOARD_ALWAYS)
	{
		spGetInput()->button[MY_BUTTON_START] = 0;
		window->feedback(selElem,WN_ACT_START_POLL);
	}
	
	int i;
	if (spGetInput()->axis[0] < 0)
	{
		if (selElem->type == 0)
		{
			spGetInput()->axis[0] = 0;
			window->feedback(selElem,WN_ACT_LEFT);
		}
		else
		if (selElem->type == 2)
			for (i = 0; i < steps;i++)
				window->feedback(selElem,WN_ACT_LEFT);
	}
	if (spGetInput()->axis[0] > 0)
	{
		if (selElem->type == 0)
		{
			spGetInput()->axis[0] = 0;
			window->feedback(selElem,WN_ACT_RIGHT);
		}
		else
		if (selElem->type == 2)
			for (i = 0; i < steps;i++)
				window->feedback(selElem,WN_ACT_RIGHT);
	}
	window->feedback(selElem,WN_ACT_UPDATE);
	update_elem_width(selElem,window);
	update_window_width(window);
	return 0;
}

int modal_window(pWindow window, void ( *resize )( Uint16 w, Uint16 h ))
{
	spUnlockRenderTarget();
	window->oldScreen = spUniqueCopySurface( spGetWindowSurface() );
	spLockRenderTarget();
	pWindow save_window = recent_window;
	recent_window = window;
	int res = spLoop(window_draw,window_calc,10,resize,NULL);
	if (spIsKeyboardPolled())
	{
		pWindowElement selElem = window->firstElement;
		int nr = 0;
		while (selElem)
		{
			if (nr == window->selection)
				break;
			nr++;
			selElem = selElem->next;
		}
		window->feedback(selElem,WN_ACT_END_POLL);
	}
	recent_window = save_window;
	spDeleteSurface( window->oldScreen );
	window->oldScreen = NULL;
	return res;
}

void delete_window(pWindow window)
{
	while (window->firstElement)
	{
		pWindowElement next = window->firstElement->next;
		free(window->firstElement);
		window->firstElement = next;
	}
	free(window);
}

void draw_message(void)
{
	pWindow save_window = recent_window;
	recent_window = mg_window;
	window_draw();
	recent_window = save_window;	
}

int set_message(spFontPointer font, char* caption)
{
	if (mg_window)
		delete_window(mg_window);
	mg_window = create_window(NULL,font,caption);
	mg_window->do_flip = 0;
}

void message_box(spFontPointer font, void ( *resize )( Uint16 w, Uint16 h ), char* caption)
{
	pWindow window = create_window(NULL,font,caption);
	window->only_ok = 1;
	modal_window(window,resize);
	delete_window(window);
}

char* text_box_char = NULL;
int text_box_len = 0;

int text_box_feedback( pWindowElement elem, int action )
{
	switch (action)
	{
		case WN_ACT_START_POLL:
			spPollKeyboardInput(text_box_char,text_box_len,KEY_POLL_MASK);
			break;
		case WN_ACT_END_POLL:
			spStopKeyboardInput();
			break;
	}
	sprintf(elem->text,"%s",text_box_char);
	return 0;
}

int text_box(spFontPointer font, void ( *resize )( Uint16 w, Uint16 h ), char* caption, char* text,int len,int show_selection,int* sprite_count,int insult_button)
{
	char* save_char = text_box_char;
	int save_len = text_box_len;
	text_box_char = text;
	text_box_len = len;
	pWindow window = create_window(text_box_feedback,font,caption);
	window->insult_button = insult_button;
	if (show_selection)
	{
		window->show_selection = show_selection;
		window->sprite_count = sprite_count;
		window->height += (spGetSizeFactor()*16 >> SP_ACCURACY) + 2*font->maxheight;
	}
	add_window_element(window,1,0);
	if (insult_button)
		window->width += spGetWindowSurface()->w/6;
	int res = modal_window(window,resize);
	delete_window(window);
	text_box_char = save_char;
	text_box_len = save_len;
	return res;
}

int sprite_box(spFontPointer font, void ( *resize )( Uint16 w, Uint16 h ), char* caption,int show_selection,int* sprite_count)
{
	pWindow window = create_window(text_box_feedback,font,caption);
	if (show_selection)
	{
		window->show_selection = show_selection;
		window->sprite_count = sprite_count;
		window->height += (spGetSizeFactor()*16 >> SP_ACCURACY) + 2*font->maxheight;
	}
	int res = modal_window(window,resize);
	delete_window(window);
	return res;
}
