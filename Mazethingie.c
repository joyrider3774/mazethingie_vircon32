#include "video.h"
#include "string.h"
#include "time.h"
#include "misc.h"
#include "input.h"
#include "memcard.h"
#include "libs/DrawPrimitives/draw_primitives.h"
#include "libs/TextFonts/textfont.h"
#include "Mazethingie.h"


void ClearBit(int* bitFlag, int bit)
{
	(*bitFlag) &=  ~(1<< bit);
}

void SetBit(int* bitFlag, int bit)
{
	(*bitFlag) |= (1<< bit);
}

int BitSet(int bitFlag, int bit) 
{
	return bitFlag & (1 << bit);
}

void LoadSettings()
{
	SaveData save;
	save.w = MaxMazeWidth;
	save.h = MaxMazeHeight; 	
	if(card_is_connected())
		if(card_signature_matches( &GameSignature ))
		{
			 card_read_data( &save, sizeof(game_signature), sizeof(save) );
		}
	MazeWidth = save.w;
	MazeHeight = save.h;
	if (MazeWidth > MaxMazeWidth)
		MazeWidth = MaxMazeWidth;
	if(MazeHeight > MaxMazeHeight)
		MazeHeight = MaxMazeHeight;
}

void SaveSettings()
{
	if(card_is_connected())
		if(card_is_empty() || card_signature_matches( &GameSignature ))
		{
			SaveData save;
			save.w = MazeWidth;
			save.h = MazeHeight;
			card_write_signature( &GameSignature );
    		card_write_data( &save, sizeof(game_signature), sizeof(save) );
		}
}

void SetupFont()
{
	select_texture( TextureFullFont );
    
    // First we define define 128 consecutive regions (i.e. standard ASCII only)
    // with the same size and hotspot position, as for a constant-width font
    define_region_matrix( FirstRegionFullFont,  0,0,  21,31,  0,31,  16,8,  0 );
    
    // then we redefine some characters to have different widths
    // (note that, for this font, upper and lowercase letters are the same)
    select_region( FirstRegionFullFont + 'M' );
    define_region( 22,0,  46,31,  22,31 );
    select_region( FirstRegionFullFont + 'm' );
    define_region( 22,0,  46,31,  22,31 );
    
    select_region( FirstRegionFullFont + 'W' );
    define_region( 66,0,  90,31,  66,31 );
    select_region( FirstRegionFullFont + 'w' );
    define_region( 66,0,  90,31,  66,31 );
    
    select_region( FirstRegionFullFont + 'I' );
    define_region( 110,0,  121,31,  110,31 );
    select_region( FirstRegionFullFont + 'i' );
    define_region( 110,0,  121,31,  110,31 );
    
    select_region( FirstRegionFullFont + ' ' );
    define_region( 0,64,  15,95,  0,95 );

	FontLetters.character_height = 31;
    FontLetters.use_variable_width = true;
    textfont_read_region_widths( &FontLetters );
    
    // 2 pixels overlap between characters, 15 pixels between lines
    FontLetters.character_separation = -2;
    FontLetters.line_separation = 15;
    
    // define texture and regions for our characters
    FontLetters.texture_id = TextureFullFont;
    FontLetters.character_zero_region_id = FirstRegionFullFont;
}

void DrawMaze()
{
	
    int X,Y,I;
	//clear_screen (color_red);
    set_multiply_color(color_white);
    for (Y= 0;Y< MazeHeight;Y++)
        for(X = 0;X < MazeWidth;X++)
        {
			I = (Y*MazeWidth) + X;
            //north wall
			if (BitSet(Maze[I], 0))
            {
                draw_line(xoffset + (X+1) * BoxWidth,yoffset + (Y+1) * BoxHeight,xoffset + (X+2)*BoxWidth-1,yoffset + (Y+1)*BoxHeight);
            }

			//east wall
			if(BitSet(Maze[I], 1))
            {
                draw_line(xoffset + (X+2) * BoxWidth,yoffset + (Y+1) * BoxHeight,xoffset + (X+2)*BoxWidth,yoffset + (Y+2)*BoxHeight);
            }

			//south wall
            if(BitSet(Maze[I], 2))
            {
                draw_line(xoffset + (X+1) * BoxWidth,yoffset + (Y+2) * BoxHeight,xoffset + (X+2)*BoxWidth-1,yoffset + (Y+2)*BoxHeight);
            }

			//west wall
            if(BitSet(Maze[I], 3))
            {
                draw_line(xoffset + (X+1) * BoxWidth,yoffset + (Y+1) * BoxHeight,xoffset + (X+1)*BoxWidth,yoffset + (Y+2)*BoxHeight);
            }
        }
}

void GenerateMaze()
{
    int[4] neighbours;
    int[MaxMazeSize] cellStack;
    int cc = 0;
    int currentPoint = 0;
    int visitedRooms = 1;
    int tmp2;
    int selectedNeighbour;
    int rnd;
    
    //intial all walls value in every room we will remove bits of this value to remove walls
    memset(Maze, 0xf, MaxMazeSize);

	while (visitedRooms != MazeHeight*MazeWidth)
    {
        int neighboursFound = 0;
        int lookUpX = currentPoint % MazeWidth;
        int lookUpY = currentPoint / MazeWidth;
        int tmp  = currentPoint+1; 
        //tile has neighbour to the right which we did not handle yet
        if (( lookUpX + 1 < MazeWidth) && (Maze[tmp] == 0xf))
            neighbours[neighboursFound++] = tmp;
    
        tmp = currentPoint-1; 
        //tile has neighbour to the left which we did not handle yet
        if ((lookUpX > 0) && (Maze[tmp] == 0xf))
            neighbours[neighboursFound++] = tmp;

        tmp = currentPoint - MazeWidth; 
        //tile has neighbour the north which we did not handle yet
        if ((lookUpY > 0) && (Maze[tmp] == 0xf))
            neighbours[neighboursFound++] = tmp;

        tmp = currentPoint + MazeWidth; 
        //tile has neighbour the south which we did not handle yet
        if ((lookUpY + 1 < MazeHeight) && (Maze[tmp] == 0xf))
            neighbours[neighboursFound++] = tmp;

        switch (neighboursFound)
        {
            case 0:
                currentPoint = cellStack[--cc];
                continue;
                break;
            default:
                rnd = (int)(rand() % neighboursFound);
                break;
        }
        selectedNeighbour = neighbours[rnd];      
        tmp = (selectedNeighbour % MazeWidth);
        //tile has neighbour to the east
        if(tmp > lookUpX)
        {
            //remove west wall neighbour
            ClearBit(&Maze[selectedNeighbour], 3);
            //remove east wall tile
            ClearBit(&Maze[currentPoint], 1);
        }
        else // tile has neighbour to the west
        {
            if(tmp < lookUpX)
            {
                //remove east wall neighbour
                ClearBit(&Maze[selectedNeighbour], 1);
                //remove west wall tile
                ClearBit(&Maze[currentPoint], 3);
            }
            else // tile has neighbour to the north
            {
                tmp2 = selectedNeighbour / MazeWidth;
                if(tmp2 < lookUpY)
                {
                    //remove south wall neighbour
                    ClearBit(&Maze[selectedNeighbour], 2);
                    //remove north wall tile
                    ClearBit(&Maze[currentPoint], 0);
                }
                else // tile has neighbour to the south
                {
                    if(tmp2 > lookUpY)
                    {
                        //remove north wall neighbour
                        ClearBit(&Maze[selectedNeighbour], 0);
                        //remove south wall tile
                        ClearBit(&Maze[currentPoint], 2);
                    }
                }
            }
        }
        
        //add tile to the cellstack
        if(neighboursFound > 1)
        {
            cellStack[cc++] = currentPoint;
        } 
        //set tile to the neighbour   
        currentPoint = selectedNeighbour;
        visitedRooms++;
    }
	ClearBit(&Maze[0], 0);
    ClearBit(&Maze[(MazeWidth)*(MazeHeight)-1], 2);
    PlayerPosX = MazeWidth -1;
    PlayerPosY = MazeHeight - 1;
	xoffset = (((screen_width/BoxWidth) - (MazeWidth+2)) * BoxWidth) >> 1;
	yoffset = (((screen_height/BoxHeight) - (MazeHeight+2)) * BoxHeight) >> 1;
}

void DrawPlayer()
{
	set_multiply_color(color_red);
    draw_filled_rectangle((xoffset + ((PlayerPosX+1) * BoxWidth)), (yoffset + ((PlayerPosY + 1) * BoxHeight)+1), 
		(xoffset + ((PlayerPosX+2) * BoxWidth)-2), (yoffset + ((PlayerPosY + 2) * BoxHeight)-1));
}

void ErasePlayer()
{
	set_multiply_color(color_black);
    draw_filled_rectangle((xoffset + ((PlayerPosX+1) * BoxWidth)), (yoffset + ((PlayerPosY + 1) * BoxHeight)+1), 
		(xoffset + ((PlayerPosX+2) * BoxWidth)-2), (yoffset + ((PlayerPosY + 2) * BoxHeight)-1));
}

void TitleScreen()
{
	int MenuXPos = 200;
	int MenuYPos = 120;
	clear_screen(color_black);	
    textfont_print_centered(&FontLetters,screen_width>>1,40,"MAZE THINGIE");
    if (Selection==1)
	{
		set_multiply_color(color_red);
        textfont_print_from_left(&FontLetters,MenuXPos,MenuYPos,"Play");
	}
    else
	{
		set_multiply_color(color_white);
        textfont_print_from_left(&FontLetters, MenuXPos,MenuYPos,"Play");
	}

    int[50] Text;
    itoa(MazeWidth, &Text[0], 10);
    if(Selection == 2)
	{
		set_multiply_color(color_red);
	    textfont_print_from_left(&FontLetters,MenuXPos,MenuYPos  + 35,"Maze Width:");
		textfont_print_from_left(&FontLetters,MenuXPos+textfont_get_line_width(&FontLetters,"Maze Width:"), MenuYPos + 35,&Text[0]);
	}
    else
    {
		set_multiply_color(color_white);
	    textfont_print_from_left(&FontLetters,MenuXPos,MenuYPos + 35,"Maze Width:");
		textfont_print_from_left(&FontLetters,MenuXPos+textfont_get_line_width(&FontLetters,"Maze Width:"),MenuYPos + 35,&Text[0]);
	}

	itoa(MazeHeight, &Text[0], 10);
    if(Selection == 3)
	{
		set_multiply_color(color_red);
	    textfont_print_from_left(&FontLetters,MenuXPos,MenuYPos + 70,"Maze Height:");
		textfont_print_from_left(&FontLetters,MenuXPos+textfont_get_line_width(&FontLetters,"Maze Height:"),MenuYPos + 70,&Text[0]);
	}
    else
    {
		set_multiply_color(color_white);
	    textfont_print_from_left(&FontLetters,MenuXPos,MenuYPos + 70,"Maze Height:");
		textfont_print_from_left(&FontLetters,MenuXPos+textfont_get_line_width(&FontLetters,"Maze Height:"),MenuYPos + 70,&Text[0]);
	}

	set_multiply_color(color_white);
    textfont_print_centered(&FontLetters,screen_width>>1,300,"Created by\nWillems Davy - 2024");

	if(gamepad_up() == 1)
   		if (Selection > 1)
    	    Selection--;
        
	if(gamepad_down() == 1)
		if (Selection < 3)
        	Selection++;

	if(gamepad_button_a() == 1 || gamepad_button_start() == 1)
	{
		SaveSettings();
		GenerateMaze();
		clear_screen(color_black);
		DrawMaze();
		DrawPlayer();
	    GameState = GSGame;
	}

	if(Selection == 2)
    {
	    if (gamepad_right() % MenuSpeedTickVal == 1)
    	    if (MazeWidth < MaxMazeWidth)
            {
				MazeWidth++;
				SaveSettings();	
			}

        if (gamepad_left() % MenuSpeedTickVal == 1)
	        if(MazeWidth > 10 )
			{
    	        MazeWidth--;
				SaveSettings();
			}
        
	}

    if(Selection == 3)
    {
		if (gamepad_right() % MenuSpeedTickVal == 1)
			if (MazeHeight < MaxMazeHeight)
				MazeHeight++;
		if (gamepad_left() % MenuSpeedTickVal == 1)
			if(MazeHeight > 10 )
				MazeHeight--;
	}
}

void Game()
{
	if(gamepad_button_a() == 1 || gamepad_button_start() == 1)
	{
		GameWon = 0;
		clear_screen(color_black);
		GenerateMaze();
		DrawMaze();
		DrawPlayer();
	}

    if(gamepad_button_b() == 1)
		GameState = GSTitleScreen;

	if(!GameWon)
	{	
		if(gamepad_button_x() > 0)
		{
			int regen = 0;
			if (gamepad_left() % MenuSpeedTickVal == 1)
	        	if(MazeWidth > 10 )
				{
    	        	MazeWidth--;
					regen = 1;
				}

			if (gamepad_right() % MenuSpeedTickVal == 1)
				if (MazeWidth < MaxMazeWidth)
				{
            		MazeWidth++;
					regen = 1;
				}

			if (gamepad_down() % MenuSpeedTickVal == 1)
				if (MazeHeight < MaxMazeHeight)
				{
					MazeHeight++;
					regen = 1;
				}

			if (gamepad_up() % MenuSpeedTickVal == 1)
				if(MazeHeight > 10 )
				{
					MazeHeight--;
					regen = 1;
				}					
			
			if(regen)
			{
				SaveSettings();
				GameWon = 0;
				clear_screen(color_black);
				GenerateMaze();
				DrawMaze();
				DrawPlayer();
			}
		}
		else
		{
			if (gamepad_right() % PlayerSpeedTickVal == 1)
				if (!(Maze[(PlayerPosY*MazeWidth) + PlayerPosX] & 2))
				{
					ErasePlayer();
					PlayerPosX++;
					DrawPlayer();
				}
			if (gamepad_left() % PlayerSpeedTickVal == 1)
				if (!(Maze[(PlayerPosY*MazeWidth) + PlayerPosX] & 8))
				{
					ErasePlayer();
					PlayerPosX--;
					DrawPlayer();
				}

			if (gamepad_up() % PlayerSpeedTickVal == 1)
			{    
				if((PlayerPosX == 0) && (PlayerPosY == 0))
				{
					GameWon = 1;                                                         
					int tw = textfont_get_line_width(&FontLetters, "let's try another one");
					int th = (FontLetters.character_height + FontLetters.line_separation) * 3;
					set_multiply_color(color_black);
					draw_filled_rectangle(((screen_width - tw) >> 1) - 10, ((screen_height - th ) >> 1) - 10, ((screen_width + tw ) >> 1) + 10, ((screen_height + th ) >> 1 ) + 10);
					set_multiply_color(color_red);
					draw_rectangle(((screen_width - tw) >> 1) - 10 + 3 , ((screen_height - th) >> 1) - 10 + 3, ((screen_width + tw) >> 1) + 10 -3, ((screen_height + th) >> 1) + 10 - 3);
					set_multiply_color(color_white);
					textfont_print_centered(&FontLetters, screen_width >> 1, ((screen_height - th +20 + FontLetters.character_height) >> 1) + 10,"Congratulations !!!\nYou solved the maze,\nlet's try another one");
				}
				else
					if (!(Maze[(PlayerPosY*MazeWidth) + PlayerPosX] & 1))
					{
						ErasePlayer();
						PlayerPosY--;
						DrawPlayer();
					}
			}

			if (gamepad_down() % PlayerSpeedTickVal == 1)
				if((PlayerPosX != MazeWidth-1) || (PlayerPosY !=MazeHeight -1))
					if (!(Maze[(PlayerPosY*MazeWidth) + PlayerPosX] & 4))
					{
						ErasePlayer();
						PlayerPosY++;
						DrawPlayer();
					}
		}
	}
}

void main()
{  
	memset( &GameSignature, 0, sizeof( game_signature ) );
    strcpy( GameSignature, "MAZETHINGIE" );
	LoadSettings();
	SetupFont();	
	srand(get_time());
    select_gamepad(0);
	while (true) 
    {
      switch (GameState) 
	  {
		case GSTitleScreen:
			TitleScreen();
			break;
		case GSGame:
			Game();
			break;
		default:
			break;
	  }
      end_frame();
    }
	framecount++;
	if (framecount >60000)
		framecount = 0;
}