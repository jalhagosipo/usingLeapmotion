#include <iostream>
#include <fstream>
#include "Leap.h"
#include <math.h>
#include "GL/glut.h"
#include <deque>
#include<Windows.h>
#include<math.h>
#include <WinUser.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "winmm")

#define HORZRES 8
#define VERTRES 10
#define MOUSEEVENTF_RIGHTDOWN 0x0008
#define MOUSEEVENTF_RIGHTUP 0x0010
#define MOUSEEVENTF_LEFTDOWN 0x0002
#define MOUSEEVENTF_LEFTUP 0x0004
#define MOUSEEVENTF_WHEEL  0x0800
#define MOUSEEVENTF_HWHEEL 0x01000
#define WM_LBUTTONDBLCLK 0x0203

#define SCROLLUP 120
#define SCROLLDOWN -120

using namespace Leap;
using namespace std;

//IAudioEndpointVolume

double screenWidth = 300, screenHeight = 300;//hand graphic window size
int windowWidth = (int)GetDeviceCaps(GetDC(NULL), VERTRES);//get window width
int windowHeight = (int)GetDeviceCaps(GetDC(NULL), HORZRES);//get window height

double fing[2][5][3];
float phi[2][5][4] = { 0 };
float blen[2][5][4] = { 0 };
float theta[2][5][4] = { 0 };
float boneStart[2][5][4][3] = { 0 };
float boneEnd[2][5][4][3] = { 0 };
double wris[2][3] = { 0 };
double pal[2][3] = { 0 };
string cur_state = "cur", prev_state = "prev";

Frame frame;
HandList hands;
FingerList fingers;

int lc, rc;//which hand
bool gesture_expand;
bool gesture_reduction;
bool reduction_expand_check = false;
bool action = false;
bool dblClick = false;
int cnt = 0;//to control expand_reduction
float cursorX, cursorY;

// Pointer for drawing quadratoc objects
GLUquadricObj *quadratic;


class SampleListener : public Listener {
public:
	virtual void onInit(const Controller&);
	virtual void onConnect(const Controller&);
	virtual void onExit(const Controller&);
	virtual void onFrame(const Controller&);
	float moveScale = 0.5;
	//bool isReversed = false;

};

void SampleListener::onInit(const Controller& controller) {
	cout << "Initialized.\n";
}

void SampleListener::onConnect(const Controller& controller) {
	cout << "Connected.\n";
}

void SampleListener::onExit(const Controller& controller) {
	cout << "Exited.\n";
}

void SampleListener::onFrame(const Controller& controller) {

	int j = 0;//save to list which hand
	frame = controller.frame();
	lc = 0;//left hand
	rc = 0;//right hand
	controller.enableGesture(Leap::Gesture::TYPE_SWIPE);
	controller.enableGesture(Leap::Gesture::TYPE_CIRCLE);
	controller.enableGesture(Leap::Gesture::TYPE_KEY_TAP);

	GestureList gestures = frame.gestures();

	hands = frame.hands();

	for (HandList::const_iterator hl = hands.begin(); hl != hands.end(); ++hl) {

		// Get the first hand
		const Hand hand = *hl;

		int	handtype = (hand.isLeft() ? 0 : 1);
		j = handtype;

		if (j == 0) { lc = 1; }
		else { rc = 1; }

		// Get the Arm bone
		Arm arm = hand.arm();

		wris[j][0] = arm.wristPosition().x;
		wris[j][1] = arm.wristPosition().y;
		wris[j][2] = arm.wristPosition().z;

		pal[j][0] = hand.palmPosition().x;
		pal[j][1] = hand.palmPosition().y;
		pal[j][2] = hand.palmPosition().z;


		// Get fingers
		fingers = hand.fingers();

		for (FingerList::const_iterator fl = fingers.begin(); fl != fingers.end(); ++fl) {
			const Finger finger = *fl;

			fing[j][finger.type()][0] = finger.tipPosition().x;
			fing[j][finger.type()][1] = finger.tipPosition().y;
			fing[j][finger.type()][2] = finger.tipPosition().z;

			// Get joints & bones
			for (int b = 0; b < 4; ++b) {
				Bone::Type boneType = static_cast<Bone::Type>(b);
				Bone bone = finger.bone(boneType);

				boneStart[j][finger.type()][b][0] = bone.prevJoint().x;
				boneStart[j][finger.type()][b][1] = bone.prevJoint().y;
				boneStart[j][finger.type()][b][2] = bone.prevJoint().z;

				boneEnd[j][finger.type()][b][0] = bone.nextJoint().x;
				boneEnd[j][finger.type()][b][1] = bone.nextJoint().y;
				boneEnd[j][finger.type()][b][2] = bone.nextJoint().z;


				float boneVectorX = boneEnd[j][finger.type()][b][0] - boneStart[j][finger.type()][b][0];
				float boneVectorY = boneEnd[j][finger.type()][b][1] - boneStart[j][finger.type()][b][1];
				float boneVectorZ = boneEnd[j][finger.type()][b][2] - boneStart[j][finger.type()][b][2];

				//directioin,angle
				phi[j][finger.type()][b] = atan2(boneVectorX, boneVectorZ) * 180 / PI;
				theta[j][finger.type()][b] = (-1) * atan2(boneVectorY, hypot(boneVectorX, boneVectorZ)) * 180 / PI;
				blen[j][finger.type()][b] = bone.length();

			}
		}

	}


	//////////////////////////////////////////////////////////////////cursor positioin.only right
	if (action == false)
	{
		if (hands[0].isRight())
		{
			//Device x = -60 ～ 60
			cursorX = (hands[0].palmPosition().x + 60) / 120 * windowWidth * moveScale;;
			//Device y =  110~170((((200~300
			cursorY = (1 - (hands[0].palmPosition().y - 200) / 100) * windowHeight * moveScale;

			SetCursorPos((int)cursorX, (int)cursorY);
		}
	}


	
	////////////////////////////////////////////////////////////////////////////////////////////////////
	/*gesture check*/
	////////////////////////////////////////////////////////////////////////////////////////////////////
	if (/*!reduction_expand_check &&*/ hands.count() == 1)
	{
		///////////////////////////////////////////////////////////////////////////////Start
		///////////doubleclick_Start
		if (!dblClick && !gesture_expand && !gesture_reduction&&
			hands[0].grabAngle() == PI &&
			!fingers[0].isExtended() &&
			2 * hands[0].sphereRadius() < 75.0
			&& hands[0].direction().up().angleTo(hands[0].palmNormal()) > 2.5)
		{
			mouse_event(MOUSEEVENTF_LEFTDOWN, cursorX, cursorY, 0, 0);
			mouse_event(MOUSEEVENTF_LEFTUP, cursorX, cursorY, 0, 0);
			mouse_event(MOUSEEVENTF_LEFTDOWN, cursorX, cursorY, 0, 0);
			dblClick = true;
			cout << "dblClick_True" << endl;
		}
		//////////////////////gesture_expand_Start
		else if (!gesture_expand && !dblClick && !gesture_reduction&&
			abs(fing[1][0][0] - fing[1][1][0]) < 20 &&
			abs(fing[1][0][1] - fing[1][1][1]) < 20 &&
			abs(fing[1][0][2] - fing[1][1][2]) < 20 &&
			abs(fing[1][0][0] - fing[1][2][0]) < 20 &&
			abs(fing[1][0][1] - fing[1][2][1]) < 20 &&
			abs(fing[1][0][2] - fing[1][2][2]) < 20 &&
			hands[0].direction().up().angleTo(hands[0].palmNormal()) < 1.9
			)
		{
			action = true;
			gesture_expand = true;
			cout << "expand_True" << endl;
		}

		///////////////////////////reduction_Start
		else if (!gesture_reduction && !dblClick && !gesture_expand && hands[0].direction().up().angleTo(hands[0].palmNormal()) < 1.9 && hands[0].grabAngle() == 0 &&
			fingers[0].isExtended() && fingers[1].isExtended() && fingers[2].isExtended() && fingers[3].isExtended() && fingers[4].isExtended())
		{
			action = true;
			gesture_reduction = true;
			cout << "reduction_True" << endl;
		}

		///////////////////////////////////////////////////////////////////////////////Progress-Stop
		/////////////////////////////reduction_Stop
		if (gesture_reduction)
		{
			cout << "축소" << endl;

			if (hands[0].direction().up().angleTo(hands[0].palmNormal()) < 1.9 && hands[0].grabAngle() == PI
				)
			{
				action = false;
				gesture_reduction = false;
				reduction_expand_check = true;
				cout << "reduction_False" << endl;
			}
		}
		///////////////////////////expand_Stop
		else if (gesture_expand)
		{
			cout << "확대" << endl;

			if (fingers[0].isExtended() && fingers[1].isExtended() && fingers[2].isExtended() && !fingers[3].isExtended() && !fingers[4].isExtended() && abs(fing[1][3][2] - fing[1][4][2]) < 20)
			{
				action = false;
				gesture_expand = false;
				reduction_expand_check = true;
				cout << "expand_False" << endl;
			}
		}
		///////////doubleclick_Stop
		else if (dblClick && 2 * hands[0].sphereRadius() > 90.0 && hands[0].direction().up().angleTo(hands[0].palmNormal()) > 2.5)
		{
			mouse_event(MOUSEEVENTF_LEFTUP, cursorX, cursorY, 0, 0);
			cout << "더블클릭" << endl;
			dblClick = false;
			cout << "dblClick_False" << endl;
		}
		////////////////////go back
		else if (
			hands[0].direction().pitch()* (180.0 / PI) > -10 && hands[0].direction().pitch()* (180.0 / PI) < 10 &&
			hands[0].palmNormal().roll()* (180.0 / PI) > -10 && hands[0].palmNormal().roll()* (180.0 / PI) < 10 &&
			hands[0].direction().yaw()* (180.0 / PI) > -15 && hands[0].direction().yaw()* (180.0 / PI) < 5 &&
			hands[0].grabAngle() == PI &&
			fingers[0].isExtended()
			)
		{
			cout << "뒤로가기" << endl;
		}
		///////////////leftClick,rightClick,scroll,volume
		else
		{
			for (GestureList::const_iterator gl = gestures.begin(); gl != gestures.end(); ++gl)
			{
				Gesture gesture = *gl;

				if ((gesture.type() == gesture.TYPE_CIRCLE) &&
					abs(fingers[0].tipPosition().x - fingers[4].tipPosition().x) < 15 &&
					abs(fingers[0].tipPosition().y - fingers[4].tipPosition().y) < 15 &&
					abs(fingers[0].tipPosition().z - fingers[4].tipPosition().z) < 15 &&
					fingers[1].isExtended() && fingers[2].isExtended() &&fingers[3].isExtended())
				{
					CircleGesture circle = CircleGesture(gesture);

					if (circle.STATE_START)
					{
						prev_state = cur_state;
						action = true;
						cout << "volume_True" << endl;
					}

					// Calculate clock direction using the angle between circle normal and pointable
					string clockwiseness;


					// Clockwise if angle is less than 90 degrees
					if (circle.pointable().direction().angleTo(circle.normal()) <= PI / 4)
					{
						clockwiseness = "볼륨업";
					}
					else
					{
						clockwiseness = "볼륨다운";
					}
					cur_state = clockwiseness;
					cout << clockwiseness << endl;

					if (circle.STATE_STOP)
					{
						action = false;
						cout << "volume_False" << endl;
					}
				}
				else if ((gesture.type() == gesture.TYPE_SWIPE) &&
					abs(fingers[0].tipPosition().x - fingers[1].tipPosition().x) < 20 &&
					abs(fingers[0].tipPosition().y - fingers[1].tipPosition().y) < 20 &&
					abs(fingers[0].tipPosition().z - fingers[1].tipPosition().z) < 20)
				{
					string swipeDirection;
					SwipeGesture swipe = SwipeGesture(gesture);

					//Classify swipe as either horizontal or vertical
					float x = swipe.direction().x;
					float y = swipe.direction().y;

					if (swipe.STATE_START)
					{
						prev_state = cur_state;
						action = true;
						cout << "scroll_True" << endl;
					}
					if (fabsf(x) > fabsf(y))
					{
						if (x > 0.0f) {
							mouse_event(MOUSEEVENTF_HWHEEL, cursorX, cursorY, SCROLLUP, 0);
							cout << "오른쪽" << endl;
							cur_state = "오른쪽";
						}
						else {
							mouse_event(MOUSEEVENTF_HWHEEL, cursorX, cursorY, SCROLLDOWN, 0);
							cout << "왼쪽" << endl;
							cur_state = "왼쪽";
						}
					}
					else
					{
						if (y > 0.0f) {
							mouse_event(MOUSEEVENTF_WHEEL, cursorX, cursorY, SCROLLUP, 0);
							cout << "위" << endl;
							cur_state = "위";

						}
						else {
							mouse_event(MOUSEEVENTF_WHEEL, cursorX, cursorY, SCROLLDOWN, 0);
							cout << "아래" << endl;
							cur_state = "아래";
						}
					}

					if (swipe.STATE_STOP)
					{
						action = false;
						cout << "scroll_False" << endl;
					}
				}
				else if ((gesture.type() == gesture.TYPE_KEY_TAP) &&
					(fingers[1].isExtended() && !fingers[0].isExtended() && !fingers[2].isExtended() && !fingers[3].isExtended() && !fingers[4].isExtended() ||
						fingers[1].isExtended() && !fingers[0].isExtended() && fingers[2].isExtended() && !fingers[3].isExtended() && !fingers[4].isExtended()))
				{
					KeyTapGesture keytap = KeyTapGesture(gesture);

					if (keytap.STATE_START)
					{
						action = true;
						cout << "Clickcheck_ON" << endl;
					}
					if (fingers[1].isExtended() && !fingers[0].isExtended() && !fingers[2].isExtended() && !fingers[3].isExtended() && !fingers[4].isExtended())
					{
						mouse_event(MOUSEEVENTF_LEFTDOWN, cursorX, cursorY, 0, 0);
						mouse_event(MOUSEEVENTF_LEFTUP, cursorX, cursorY, 0, 0);
						cout << "왼쪽클릭" << endl;

					}
					else if (fingers[1].isExtended() && !fingers[0].isExtended() && fingers[2].isExtended() && !fingers[3].isExtended() && !fingers[4].isExtended())
					{
						mouse_event(MOUSEEVENTF_RIGHTDOWN, cursorX, cursorY, 0, 0);
						mouse_event(MOUSEEVENTF_RIGHTUP, cursorX, cursorY, 0, 0);
						cout << "오른쪽클릭" << endl;
					}
					if (keytap.STATE_STOP)
					{
						action = false;
						cout << "Clickcheck_OFF" << endl;
					}
				}
				else { cur_state = "cur"; prev_state = "prev"; break; }
			}
		}


	}


	/////////////////////////////////////////hold cursor during volumechange or scroll
	if (cur_state == prev_state)
	{
		action = true;
	}
	else
	{
		action = false;
	}

	/*if (reduction_expand_check) {

		if (cnt == 20) { reduction_expand_check = false; cnt = 0; }
		cnt++;
	}*/
}


//Create a sample listener and controller
SampleListener listener;
Controller controller;

GLfloat mat_specular[] = { 1.0, 1.0,1.0,1.0 };
GLfloat mat_shininess[] = { 50.0 };
GLfloat light_position[] = { 1.0,1.0,1.0,0.0 };

void draw(int n) {
	//wrist
	glPushMatrix();
	glTranslatef(wris[n][0], wris[n][1], wris[n][2]);
	glColor3f(0.0, 0.0, 1.0);
	glutSolidSphere(8, 50, 50);
	glPopMatrix();

	//palm position
	glPushMatrix();
	glTranslatef(pal[n][0], pal[n][1], pal[n][2]);
	glutSolidSphere(10, 50, 50);
	glPopMatrix();


	for (int i = 0; i < 5; i++)
	{

		glPushMatrix();
		glTranslated(fing[n][i][0], fing[n][i][1], fing[n][i][2]);
		glutSolidSphere(6, 50, 50);
		glPopMatrix();

		for (int b = 0; b < 4; b++)
		{
			// Draw joints
			glPushMatrix();
			glTranslatef(boneStart[n][i][b][0], boneStart[n][i][b][1], boneStart[n][i][b][2]);
			glutSolidSphere(5, 50, 50);
			glPopMatrix();

			glPushMatrix();
			glTranslatef(boneStart[n][i][b][0], boneStart[n][i][b][1], boneStart[n][i][b][2]);
			glRotatef(phi[n][i][b], 0.0f, 1.0f, 0.0f);
			glRotatef(theta[n][i][b], 1.0f, 0.0f, 0.0f);
			quadratic = gluNewQuadric();
			gluCylinder(quadratic, 1, 1, blen[n][i][b], 32, 32);
			glPopMatrix();
		}
	}
}

void display(void)
{

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	gluLookAt(0.0, 900.0, 500.0, 0.0, 100.0, 100.0, 0.0, 1.0, 0.0);//(x,y,z) eye : 카메라의 위치 at : 카메라가 보는 곳의 위치 up : 카메라 머리윗방향 벡터
	glShadeModel(GL_SMOOTH);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);    // This has to be light not material
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_COLOR);
	glColor3f(1.0, 1.0, 1.0);


	//if detected hand is left
	if (lc == 1)
	{
		draw(0);
	}
	if (rc == 1)
	{
		draw(1);
	}

	glFlush();
	glutSwapBuffers();
}

void idle(void)
{
	glutPostRedisplay();
}

void myKeys(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 'q':   // Quit
				// Remove the sample listener when done
		controller.removeListener(listener);

		exit(0);
	}
}

void resize(int w, int h)//윈도우크기변화에따라비율조절
{
	//창이 아주 작을 때, 0 으로 나누는 것을 예방합니다.
	if (h == 0)
		h = 1;
	float ratio = 1.0* w / h;

	//좌표계를 수정하기 전에 초기화합니다.
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	//뷰포트를 창의 전체 크기로 설정합니다.
	glViewport(0, 0, w, h);

	//투시값을 설정합니다.
	gluPerspective(45, ratio, 1, 1000);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void main(int argc, char **argv)
{

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(300, 300);
	glutCreateWindow("test");
	glutReshapeFunc(resize);
	glutDisplayFunc(display);

	glutKeyboardFunc(myKeys);//키보드콜백함수등록,키보드가눌러지면호출됨
	glutIdleFunc(idle);//처리하고있는 이벤트가없을때 실행될함수를설정하는함수.
	glEnable(GL_DEPTH_TEST);

	controller.addListener(listener);
	glutMainLoop();
}
