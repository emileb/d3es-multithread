



extern int main_android(int argc, char *argv[]);

extern "C"
{

#include "game_interface.h"
#include "SDL.h"
#include "SDL_keycode.h"

#define ACTION_DOWN 0
#define ACTION_UP 1
#define ACTION_MOVE 2
#define ACTION_MOVE_REL 3
#define ACTION_HOVER_MOVE 7
#define ACTION_SCROLL 8
#define BUTTON_PRIMARY 1
#define BUTTON_SECONDARY 2
#define BUTTON_TERTIARY 4
#define BUTTON_BACK 8
#define BUTTON_FORWARD 16

static char* consoleCmd = NULL;

extern int SDL_SendKeyboardKey(Uint8 state, SDL_Scancode scancode);
void Android_OnMouse( int androidButton, int action, float x, float y);


int PortableKeyEvent(int state, int code, int unicode){

	LOGI("PortableKeyEvent %d %d %d",state,code,unicode);

	if (state)
		SDL_SendKeyboardKey(SDL_PRESSED, (SDL_Scancode)code);
	else
		SDL_SendKeyboardKey(SDL_RELEASED, (SDL_Scancode) code);

	return 0;

}

void PortableBackButton()
{
    PortableKeyEvent(1, SDL_SCANCODE_ESCAPE,0 );
    PortableKeyEvent(0, SDL_SCANCODE_ESCAPE, 0);
}

static const char *cmd_to_run = NULL;
void PortableCommand(const char * cmd)
{
	cmd_to_run = cmd;
}

// Can only set one impulse per frame, this should be fine
static int nextImpulse = 0;
static void SetImpuse(int impulse)
{
	nextImpulse = impulse;
}

typedef enum {
	UB_NONE,

	UB_UP,
	UB_DOWN,
	UB_LEFT,
	UB_RIGHT,
	UB_FORWARD,
	UB_BACK,
	UB_LOOKUP,
	UB_LOOKDOWN,
	UB_STRAFE,
	UB_MOVELEFT,
	UB_MOVERIGHT,

	UB_BUTTON0,
	UB_BUTTON1,
	UB_BUTTON2,
	UB_BUTTON3,
	UB_BUTTON4,
	UB_BUTTON5,
	UB_BUTTON6,
	UB_BUTTON7,

	UB_ATTACK,
	UB_SPEED,
	UB_ZOOM,
	UB_SHOWSCORES,
	UB_MLOOK,

	UB_IMPULSE0,
	UB_IMPULSE1,
	UB_IMPULSE2,
	UB_IMPULSE3,
	UB_IMPULSE4,
	UB_IMPULSE5,
	UB_IMPULSE6,
	UB_IMPULSE7,
	UB_IMPULSE8,
	UB_IMPULSE9,
	UB_IMPULSE10,
	UB_IMPULSE11,
	UB_IMPULSE12,
	UB_IMPULSE13,
	UB_IMPULSE14,
	UB_IMPULSE15,
	UB_IMPULSE16,
	UB_IMPULSE17,
	UB_IMPULSE18,
	UB_IMPULSE19,
	UB_IMPULSE20,
	UB_IMPULSE21,
	UB_IMPULSE22,
	UB_IMPULSE23,
	UB_IMPULSE24,
	UB_IMPULSE25,
	UB_IMPULSE26,
	UB_IMPULSE27,
	UB_IMPULSE28,
	UB_IMPULSE29,
	UB_IMPULSE30,
	UB_IMPULSE31,
	UB_IMPULSE32,
	UB_IMPULSE33,
	UB_IMPULSE34,
	UB_IMPULSE35,
	UB_IMPULSE36,
	UB_IMPULSE37,
	UB_IMPULSE38,
	UB_IMPULSE39,
	UB_IMPULSE40,
	UB_IMPULSE41,
	UB_IMPULSE42,
	UB_IMPULSE43,
	UB_IMPULSE44,
	UB_IMPULSE45,
	UB_IMPULSE46,
	UB_IMPULSE47,
	UB_IMPULSE48,
	UB_IMPULSE49,
	UB_IMPULSE50,
	UB_IMPULSE51,
	UB_IMPULSE52,
	UB_IMPULSE53,
	UB_IMPULSE54,
	UB_IMPULSE55,
	UB_IMPULSE56,
	UB_IMPULSE57,
	UB_IMPULSE58,
	UB_IMPULSE59,
	UB_IMPULSE60,
	UB_IMPULSE61,
	UB_IMPULSE62,
	UB_IMPULSE63,

	UB_MAX_BUTTONS
} usercmdButton_t;

static int cmdButtons[UB_MAX_BUTTONS];


void buttonChange(int state, int key )
{
	cmdButtons[key] = state;
}

void PortableAction(int state, int action)
{
	LOGI("PortableAction %d   %d",state,action);


	if ((action >= PORT_ACT_CUSTOM_0) && (action <= PORT_ACT_CUSTOM_17))
    {
        if( action <= PORT_ACT_CUSTOM_9 )
            PortableKeyEvent(state, SDL_SCANCODE_KP_1 + action - PORT_ACT_CUSTOM_0, 0);
        else if(action <= PORT_ACT_CUSTOM_17)
             PortableKeyEvent(state, SDL_SCANCODE_A + action - PORT_ACT_CUSTOM_10, 0);
    }
	else if(( PortableGetScreenMode() == TS_MENU ) || ( PortableGetScreenMode() == TS_BLANK ))
	{
		if (action >= PORT_ACT_MENU_UP && action <= PORT_ACT_MENU_BACK)
		{

			int sdl_code [] = { SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_LEFT,
					SDL_SCANCODE_RIGHT, SDL_SCANCODE_RETURN, SDL_SCANCODE_ESCAPE };
			PortableKeyEvent(state, sdl_code[action-PORT_ACT_MENU_UP], 0);
			return;
		}
	}
    else
	{
        switch (action)
        {
        case PORT_ACT_LEFT:
            buttonChange(state,UB_LEFT);
            break;
        case PORT_ACT_RIGHT:
            buttonChange(state,UB_RIGHT);
            break;
        case PORT_ACT_FWD:
            buttonChange(state,UB_FORWARD);
            break;
        case PORT_ACT_BACK:
            buttonChange(state,UB_BACK);
            break;
        case PORT_ACT_MOVE_LEFT:
            buttonChange(state,UB_MOVELEFT);
            break;
        case PORT_ACT_MOVE_RIGHT:
            buttonChange(state,UB_MOVERIGHT);
            break;
        case PORT_ACT_ATTACK:
            buttonChange(state,UB_ATTACK);
            break;
        case PORT_ACT_ALT_ATTACK:

            break;
        case PORT_ACT_TOGGLE_ALT_ATTACK:

			break;
        case PORT_ACT_JUMP:
            buttonChange(state,UB_UP);
            break;
        case PORT_ACT_DOWN:
            buttonChange(state,UB_DOWN);
            break;
        case PORT_ACT_TOGGLE_CROUCH:

            break;
        case PORT_ACT_NEXT_WEP:
            if (state)
				SetImpuse(UB_IMPULSE14);
            break;
        case PORT_ACT_PREV_WEP:
            if (state)
                SetImpuse(UB_IMPULSE15);
            break;
        case PORT_ACT_RELOAD:
            if (state)
                SetImpuse(UB_IMPULSE13);
            break;
      	case PORT_ACT_WEAP0:
            if (state)
                SetImpuse(UB_IMPULSE0);
            break;
        case PORT_ACT_WEAP1:
            if (state)
                SetImpuse(UB_IMPULSE1);
            break;
        case PORT_ACT_WEAP2:
            if (state)
                SetImpuse(UB_IMPULSE2);
            break;
        case PORT_ACT_WEAP3:
            if (state)
                SetImpuse(UB_IMPULSE3);
            break;
        case PORT_ACT_WEAP4:
            if (state)
                SetImpuse(UB_IMPULSE4);
            break;
        case PORT_ACT_WEAP5:
            if (state)
                SetImpuse(UB_IMPULSE5);
            break;
        case PORT_ACT_WEAP6:
            if (state)
                SetImpuse(UB_IMPULSE6);
            break;
        case PORT_ACT_WEAP7:
            if (state)
                SetImpuse(UB_IMPULSE7);
            break;
        case PORT_ACT_WEAP8:
            if (state)
                SetImpuse(UB_IMPULSE8);
            break;
        case PORT_ACT_WEAP9:
            if (state)
                SetImpuse(UB_IMPULSE9);
            break;
		case PORT_ACT_FLASH_LIGHT:
			if (state)
				SetImpuse(UB_IMPULSE11);
			break;
		case PORT_ACT_HELPCOMP:
			if (state)
				SetImpuse(UB_IMPULSE19);
			break;
        case PORT_ACT_QUICKLOAD:
			PortableKeyEvent(state,SDL_SCANCODE_F9, 0);
            break;
        case PORT_ACT_QUICKSAVE:
			PortableKeyEvent(state,SDL_SCANCODE_F5, 0);
            break;
        case PORT_ACT_CONSOLE:
            if (state)
                PortableCommand("toggleconsole");
            break;
        }
	}
}


// =================== FORWARD and SIDE MOVMENT ==============

float forwardmove_android, sidemove_android; //Joystick mode

void PortableMoveFwd(float fwd)
{
	if (fwd > 1)
		fwd = 1;
	else if (fwd < -1)
		fwd = -1;

	forwardmove_android = fwd;
}

void PortableMoveSide(float strafe)
{
	if (strafe > 1)
		strafe = 1;
	else if (strafe < -1)
		strafe = -1;

	sidemove_android = strafe;
}

void PortableMove(float fwd, float strafe)
{
	PortableMoveFwd(fwd);
	PortableMoveSide(strafe);
}

//======================================================================

//Look up and down
float look_pitch_mouse,look_pitch_abs,look_pitch_joy;
void PortableLookPitch(int mode, float pitch)
{
	switch(mode)
	{
	case LOOK_MODE_MOUSE:
		look_pitch_mouse += pitch;
		break;
	case LOOK_MODE_ABSOLUTE:
		look_pitch_abs = pitch;
		break;
	case LOOK_MODE_JOYSTICK:
		look_pitch_joy = pitch;
		break;
	}
}

//left right
float look_yaw_mouse,look_yaw_joy;
void PortableLookYaw(int mode, float yaw)
{
	switch(mode)
	{
	case LOOK_MODE_MOUSE:
		look_yaw_mouse += yaw;
		break;
	case LOOK_MODE_JOYSTICK:
		look_yaw_joy = yaw;
		break;
	}
}




void PortableInit(int argc,const char ** argv){
	memset(cmdButtons,0,sizeof(cmdButtons));

	main_android(argc,(char **)argv);
}


void PortableMouse(float dx,float dy)
{
    //LOGI("%f %f",dx,dy);
    Android_OnMouse(0, ACTION_MOVE_REL, -dx * 3000, -dy * 1200);
}

void PortableMouseButton(int state, int button, float dx,float dy)
{
	LOGI("PortableMouseButton %d", state);

	//PortableAction(state, PORT_ACT_ATTACK);
	//return;
    if( state )
        Android_OnMouse(BUTTON_PRIMARY, ACTION_DOWN, 0, 0);
    else
        Android_OnMouse(BUTTON_PRIMARY, ACTION_UP,0, 0);
}


void PortableAutomapControl(float zoom, float x, float y)
{
}

bool inMenu = false;
bool inGameGuiActive = false;
bool objectiveSystemActive = false;

touchscreemode_t PortableGetScreenMode()
{
	if(inMenu || objectiveSystemActive)
		return TS_MENU;
	else
		return TS_GAME;
}

int PortableShowKeyboard(void)
{
	return 0;
}



int Android_GetButton( int key )
{
	return cmdButtons[key];
}

int Android_GetNextImpulse()
{
	int impulse = nextImpulse;
	nextImpulse = 0;
	return impulse;
}

const char * Android_GetCommand()
{
	// Potential race condition here to miss a command, however extremely unlikely to happen
	const char *cmd = cmd_to_run;
	cmd_to_run = NULL;
	return cmd;
}

void Android_PumpEvents(int screen)
{
	inMenu = screen & 0x1;
	inGameGuiActive = !!(screen & 0x2);
	objectiveSystemActive = !!(screen & 0x4);
}

extern "C" int blockGamepad( void );

void Android_GetMovement(int frameTime, int *forward, int *strafe, float *yaw, float *pitch)
{

	//LOGI("Android_GetMovement frameTime = %d", frameTime);

	if(abs(forwardmove_android) >= 1)
	{
		buttonChange(1,UB_SPEED);
	}
	else
	{
		buttonChange(0,UB_SPEED);
	}

    int blockMove = blockGamepad() & ANALOGUE_AXIS_FWD;
    int blockLook = blockGamepad() & ANALOGUE_AXIS_PITCH;


    if( !blockMove )
    {
	    *forward = forwardmove_android * 127.0;
		*strafe = sidemove_android * 127.0;
    }

	float lookScale = inGameGuiActive ? 0.3 : 1;
    if( !blockLook  && !(inMenu || objectiveSystemActive))
    {
        // Add pitch
        *pitch += (-look_pitch_mouse * 300) * lookScale;
        look_pitch_mouse = 0;
        *pitch += (look_pitch_joy * frameTime) * lookScale / 3;

        // Add yaw
        *yaw += (look_yaw_mouse * 1000) * lookScale;
        look_yaw_mouse = 0;
        *yaw += (look_yaw_joy * frameTime) * lookScale / 2;
    }
}

}