#pragma comment(lib,"ddraw.lib") 
#pragma comment(lib,"dxguid.lib") 
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <mmsystem.h>
#include <time.h>
#include <direct.h>
#include <ddraw.h>
#include "ddutil.h"
#include "resource.h"
#include <fstream>

using namespace std;

#define WINDOW_WIDTH	350//���ڿ���
#define WINDOW_HEIGHT	450//���ڸ߶�
#define TIMER 1 //��ʱ��ID

LPDIRECTDRAW            lpDD;       //DirectDraw����
LPDIRECTDRAWCLIPPER		lpClipper;	//DirectDrawClipper����
LPDIRECTDRAWSURFACE     lpDDSFront; //���¾�ΪDirectDrawSurface����
LPDIRECTDRAWSURFACE     lpDDSBack;   
LPDIRECTDRAWSURFACE     lpDDSPlane[3];
LPDIRECTDRAWSURFACE     lpDDSBall;
LPDIRECTDRAWSURFACE     lpDDSMap;   
LPDIRECTDRAWSURFACE     lpDDSBulet[2];
LPDIRECTDRAWSURFACE     lpDDSFlame;
LPDIRECTDRAWSURFACE     lpDDSDead;
LPDIRECTDRAWSURFACE     lpDDSEnemy;   

const int BULETNUMBER=10;	//�ӵ�����
const int ENEMYNUMBER=16;	//�л�����

HWND	hWnd;				//�����ھ��
BOOL	bActive = TRUE;		//Ӧ�ó����Ƿ��Ծ
char	apppath[50];		//Ӧ�ó���·��

int		speed = 12;			//��ʱ�ٶ�
int		movespeed = 2;		//�ƶ��ٶ�

int		gamestatus;			//��Ϸ����
#define GAME_LOG	0
#define GAME_START	1
#define GAME_OVER	2

BOOL	super = FALSE;		//�һ��Ƿ��޵�
BOOL	cancontrol = FALSE;	//�һ��Ƿ��ܿ���

int		Plane = 1;			//�һ���̬
POINT	pos;				//�һ�λ��
int		forewidth, foreheight;//ǰ̨ҳ��Ŀ��Ⱥ͸߶�
RECT	backrect;			//��̨ҳ��ӳ�����
int		backwidth, backheight;//��̨ҳ��Ŀ��Ⱥ͸߶�
int		enemylost;			//�л���ʧ����
int		planeremain;		//�һ�ʣ������

POINT	buletposleft[BULETNUMBER];//����ӵ�λ��
POINT	buletposright[BULETNUMBER];//�Ҳ��ӵ�λ��
int		bulet=0;					//��ǰ�ӵ���¼

POINT	enemypos[ENEMYNUMBER];	//�л�λ��
int		enemyspeed[ENEMYNUMBER];//�л��ٶ�
int		enemydir[ENEMYNUMBER];	//�л�����
int		enemystyle[ENEMYNUMBER];//�л����ӵ����Ƿɻ�
int		enemydead[ENEMYNUMBER]; //�л��Ƿ�����

POINT	flamepos[ENEMYNUMBER];	//����λ��
int		flamestatus[ENEMYNUMBER];//����״̬��¼

POINT	deadflamepos;			//�һ���ʱ�Ļ���λ��
int		deadflamestatus=0;		//����״̬��¼
int		timeSurvive;

//��������
LRESULT CALLBACK WinProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
HWND InitWindow( HINSTANCE hInstance, int nCmdShow );
BOOL InitDDraw( void );
void FreeDDraw( void );
void Flip(void);
void DrawPlane(void);
void DrawMap(void );
void DrawText(void );
void UpdateFrame(void );
void ChangeMap(void);
void DrawBulet(void);
void DrawFlame(void);
void DrawEnemy(void);
void CheckHit(void);
BOOL OutOfRange(POINT p);
void GetRect(RECT* rect, long left, long top, long width, long height);

//*******************************************************************
//Ӧ�ó������
int PASCAL WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
                        LPSTR lpCmdLine, int nCmdShow)
{
	MSG		msg;

	//��ʼ��������
	hWnd=InitWindow( hInstance, nCmdShow ) ;
	if (hWnd == NULL )
		return FALSE;
	
	//��ʼ��DirectDraw��������ʵ��DirectDraw����
	if (!InitDDraw())
	{
		MessageBox(GetActiveWindow(), "��ʼ��DirectDraw�����г������������Ƿ���ȷ�İ�װ��DirectX��", "Error", MB_OK );
		FreeDDraw();
		return FALSE;
	}
	
	while(1)
	{
		if(PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			if(!GetMessage(&msg, NULL, 0, 0 )) return msg.wParam;
			TranslateMessage(&msg); 
			DispatchMessage(&msg);
		}
		else if(bActive)
		{
			//����֡Ƶ��
			static int NewCount, OldCount=0;
			NewCount = GetTickCount(); 
			if (NewCount > OldCount+speed)
			{
				OldCount=NewCount;
				UpdateFrame();//���»���
			}
		}
		//�ȴ���Ϣ
		else WaitMessage();
	}
}

//******************************************************************
//���������ڡ�
HWND InitWindow( HINSTANCE hInstance, int nCmdShow )
{
    HWND				hwnd;	
    WNDCLASS			wc;		

	//��䴰����ṹ
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WinProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon( hInstance, MAKEINTRESOURCE(IDI_ICON1) );
    wc.hCursor = LoadCursor( NULL, IDC_ARROW );
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName = NULL;
	wc.lpszClassName = "Survive";

	//ע�ᴰ����
    RegisterClass( &wc );
    
	//����������
    hwnd = CreateWindowEx(
		0,
		"Survive",
		"��Ů���ͻ�20��",
		WS_OVERLAPPEDWINDOW|WS_VISIBLE|WS_SYSMENU,
		(GetSystemMetrics(SM_CXFULLSCREEN)-WINDOW_WIDTH)/2,
		(GetSystemMetrics(SM_CYFULLSCREEN)-WINDOW_HEIGHT)/2,
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		NULL,
		NULL,
		hInstance,
		NULL );

    if( !hwnd )		return FALSE;

	//��ʾ�����´���
    ShowWindow( hwnd, nCmdShow );
    UpdateWindow( hwnd );

	return hwnd;
}

//*****************************
//������������Ϣ
LRESULT CALLBACK WinProc( HWND hWnd, UINT message, 
                            WPARAM wParam, LPARAM lParam )
{
    switch( message )
    {
	case WM_ACTIVATEAPP://
		bActive = wParam;
		break;
    case WM_KEYDOWN://������Ϣ
        switch( wParam )
        {
        case VK_ESCAPE://�˳�
            PostMessage(hWnd, WM_CLOSE, 0, 0);
            break;
		case VK_RETURN :
			if (gamestatus!= GAME_START)//��ʼ��Ϸ�����г�ʼ��
			{
				for (int k = 0; k<ENEMYNUMBER; k++)
					enemydead[k] = 0;
				enemylost = 0;
				planeremain = 0;
				gamestatus = GAME_START;
				movespeed = 2;
				timeSurvive  =  0;
				pos.x = backwidth / 2 - 25;
				pos.y = backrect.bottom;
				SetTimer(hWnd, TIMER, 1000, NULL);
			}			
			break;		
		}
        break;
	case WM_TIMER://��ʱ����Ϣ
		switch (wParam)
		{
		case TIMER:
			timeSurvive++;
			break;
		}
		break;
    case WM_DESTROY://�˳���Ϣ
        FreeDDraw();//�ͷ�����DirectDraw����
		//�ر�midi				
        PostQuitMessage( 0 );
        break;
    }
	
	//����ȱʡ��Ϣ��������
    return DefWindowProc(hWnd, message, wParam, lParam);
}

//******************************************************************
//��ʼ��DirectDraw����
BOOL InitDDraw(void)
{
	DWORD dwFlags;
	DDSURFACEDESC ddsd;

	//����DirectDraw����
	DirectDrawCreate(NULL, &lpDD, NULL);

	//����Э������Ϊ����ģʽ
	dwFlags = DDSCL_NORMAL;
	lpDD->SetCooperativeLevel(hWnd, dwFlags);

	//������ҳ��
	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	lpDD->CreateSurface(&ddsd, &lpDDSFront, NULL);

	//��������ҳ��
	ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;    
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
		RECT rect;
		GetClientRect(hWnd, &rect);
		forewidth = rect.right-rect.left;	
		foreheight = rect.bottom-rect.top;
		GetRect(&backrect, 65, 65, forewidth, foreheight);
	ddsd.dwWidth  =  backwidth = forewidth + 130;
	ddsd.dwHeight = backheight = foreheight + 130;
	lpDD->CreateSurface( &ddsd, &lpDDSBack, NULL );

	//������ͼҳ��
	ddsd.dwWidth = 64;
	ddsd.dwHeight = 64;
	lpDD->CreateSurface( &ddsd, &lpDDSMap, NULL );
	DDReLoadBitmap(lpDDSMap, "map1.bmp");

	//�����ɻ�ҳ��
	ddsd.dwWidth = 50;
	ddsd.dwHeight = 60;
	lpDD->CreateSurface( &ddsd, &lpDDSPlane[0], NULL );
	lpDD->CreateSurface( &ddsd, &lpDDSPlane[1], NULL );
	lpDD->CreateSurface( &ddsd, &lpDDSPlane[2], NULL );

	DDReLoadBitmap(lpDDSPlane[0], "left.bmp");
	DDReLoadBitmap(lpDDSPlane[1], "center.bmp");
	DDReLoadBitmap(lpDDSPlane[2], "right.bmp");
	
	DDSetColorKey(lpDDSPlane[0],RGB(0,0,0));
	DDSetColorKey(lpDDSPlane[1],RGB(0,0,0));
	DDSetColorKey(lpDDSPlane[2],RGB(0,0,0));
	
	
	//�����л�
	ddsd.dwWidth = 35;
	ddsd.dwHeight = 35;
	lpDD->CreateSurface( &ddsd, &lpDDSEnemy, NULL );
	DDReLoadBitmap(lpDDSEnemy, "enemy.bmp");
	DDSetColorKey(lpDDSEnemy,RGB(0,0,0));

	//��������ҳ��
	ddsd.dwWidth = 540;
	ddsd.dwHeight = 50;
	lpDD->CreateSurface( &ddsd, &lpDDSFlame, NULL );
	DDReLoadBitmap(lpDDSFlame, "flame.bmp");
	DDSetColorKey(lpDDSFlame,RGB(0,0,0));

	//�����һ�ײ�ٻ���ҳ��
	ddsd.dwWidth = 528;
	ddsd.dwHeight = 66;
	lpDD->CreateSurface( &ddsd, &lpDDSDead, NULL );
	DDReLoadBitmap(lpDDSDead, "flamedead.bmp");
	DDSetColorKey(lpDDSDead,RGB(0,0,0));

	//�����л��ӵ�ҳ��
	ddsd.dwWidth = 8;
	ddsd.dwHeight = 8;
	lpDD->CreateSurface( &ddsd, &lpDDSBall, NULL );
	DDReLoadBitmap(lpDDSBall, "ball.bmp");
	DDSetColorKey(lpDDSBall,RGB(0,0,0));

	//�����һ��ӵ�ҳ��
	ddsd.dwWidth = 5;
	ddsd.dwHeight = 13;
	lpDD->CreateSurface( &ddsd, &lpDDSBulet[0], NULL );
	lpDD->CreateSurface( &ddsd, &lpDDSBulet[1], NULL );

	DDReLoadBitmap(lpDDSBulet[0], "bulet0.bmp");
	DDReLoadBitmap(lpDDSBulet[1], "bulet1.bmp");

	DDSetColorKey(lpDDSBulet[0],RGB(0,0,0));
	DDSetColorKey(lpDDSBulet[1],RGB(0,0,0));

	//����Clipper�ü���
	lpDD->CreateClipper(0, &lpClipper, NULL);
	lpClipper->SetHWnd(0, hWnd);
	lpDDSFront->SetClipper(lpClipper);

	//��ʼ��Ϸ��ʼ��
	for (int k = 0; k < ENEMYNUMBER; k++) 
		enemydead[k] = 0;
	enemylost = 0;
	planeremain = 0;
	gamestatus = GAME_LOG;
	//���ö�ʱ��
	SetTimer(hWnd, TIMER, 1000, NULL);

	return TRUE;
} 

//*****************************************************
//�ͷ����е�DirectDraw����
void FreeDDraw( void )
{
	if( lpDD != NULL )
    {
        if( lpDDSFront != NULL )
        {
            lpDDSFront->Release(); lpDDSFront = NULL;
        }
		if( lpDDSBack!= NULL )
        {
            lpDDSBack->Release(); lpDDSBack= NULL;
        }
		for (int i = 0 ;i<3; i++)
		{
			if( lpDDSPlane[i]!= NULL )
			{
				lpDDSPlane[i]->Release(); lpDDSPlane[i] = NULL;
			}
		}
		
		for (int i = 0 ;i<2; i++)
		{
			if( lpDDSBulet[i] != NULL )
			{
				lpDDSBulet[i]->Release();	lpDDSBulet[i] = NULL;
			}
		}
        if( lpDDSMap != NULL )
        {
            lpDDSMap->Release(); lpDDSMap = NULL;
        }
        if( lpDDSFlame != NULL )
        {
            lpDDSFlame->Release(); lpDDSFlame = NULL;
        }
        if( lpDDSEnemy != NULL )
        {
            lpDDSEnemy->Release(); lpDDSEnemy = NULL;
        }
        if( lpDDSDead != NULL )
        {
            lpDDSDead->Release(); lpDDSDead = NULL;
        }
        if( lpDDSBall != NULL )
        {
            lpDDSBall->Release(); lpDDSBall = NULL;
        }
        lpDD->Release();
        lpDD = NULL;
    }
}

//*****************************************************
//��ҳ
void Flip(void)
{
	RECT Window;
	POINT pt;
	GetClientRect(hWnd, &Window);
	pt.x = pt.y = 0;
	ClientToScreen(hWnd, &pt);
	OffsetRect(&Window, pt.x, pt.y);

	HRESULT rval = lpDDSFront->Blt(&Window, lpDDSBack, &backrect, DDBLT_WAIT, NULL);
	if(rval == DDERR_SURFACELOST) lpDDSFront->Restore();

	return ;
}

//**********************************
//�����һ�
void DrawPlane( void )
{
	if (gamestatus != GAME_START) return;//��Ϸ����

	static int planenumber;
	static unsigned int oldtime = 0;

	if (planenumber != planeremain)
	{
		oldtime = GetTickCount();
		planenumber = planeremain;
		super = TRUE;
		cancontrol = FALSE;
		pos.x = backwidth / 2 - 25;
		pos.y = backrect.bottom;
	}
	if (super == TRUE)
		if (GetTickCount()>oldtime+3000)//�޵�ʱ��3��
			super = FALSE;

	if (cancontrol == FALSE)
	{
		if (pos.y>backrect.bottom-100) //�ɻ��ճ���ʱ���޷����Ƶ�
		{
			pos.y -= movespeed;
			Plane = 1;
		}
		else
			cancontrol = TRUE;
	}
	else
	{
		//��ⷽ���
		Plane = 1;
		if(GetAsyncKeyState(VK_UP))		{pos.y = pos.y - movespeed;}
		if(GetAsyncKeyState(VK_DOWN))	{pos.y = pos.y+movespeed;}
		if(GetAsyncKeyState(VK_LEFT))	{pos.x = pos.x-movespeed;Plane = 0;}
		if(GetAsyncKeyState(VK_RIGHT))	{pos.x = pos.x+movespeed;Plane = 2;}
	
		//��Χ���
		if (pos.x<65)	pos.x = 65;
		if (pos.x>backwidth-65-50)	pos.x = backwidth - 65 - 50;
		if (pos.y<65)	pos.y = 65;
		if (pos.y>backheight - 65 - 60)	pos.y = backheight - 65 - 60;
	}

	if (super == TRUE) //�޵�ʱ����˸Ч��
	{
		static int ot = 0;//��ʱ
		if (GetTickCount() - ot > 30)
			ot = GetTickCount();
		else
			return;
	}

	//���Ʒɻ�
	RECT rect;
	GetRect(&rect, pos.x, pos.y, 50, 60);
	HRESULT rval = lpDDSBack->Blt( &rect, lpDDSPlane[Plane], NULL, DDBLT_WAIT|DDBLT_KEYSRC , NULL);
	if(rval == DDERR_SURFACELOST)    lpDDSBack->Restore();
}

//***************************************
//���Ʊ�����ͼ
void DrawMap(void )
{
	//������ͼ��ƫ������
	static int mapposX = 64, mapposY = 64;
	mapposX--;
	if (mapposX <= 0 ) mapposX = 64;
	
	if (Plane == 0) mapposY--;
	if (Plane == 2) mapposY++;
	if (mapposY <= 0) mapposY = 64;
	if (mapposY > 64) mapposY = 1;

	for (int x = 0; x <= (int)backwidth / 64; x++)
	for (int y = 0; y <= (int)backheight / 64; y++)
	{
		RECT rect;
		GetRect(&rect, 64*x-mapposY, 64*y-mapposX, 64, 64);

		HRESULT rval = lpDDSBack->Blt( &rect, lpDDSMap, NULL, DDBLT_WAIT, NULL);
		if(rval == DDERR_SURFACELOST)    lpDDSBack->Restore();
	}
}

//************************************
//����˵�����ּ�ͼ��
void DrawText(void )
{
	HRESULT rval;
	RECT rect;
	HDC hdc;
	char temp[100];
	
	if (gamestatus == GAME_LOG)//��ʼ����
	{
		lpDDSBack->GetDC(&hdc);
		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, RGB(0,255,0)); 
			sprintf(temp, "��Ů���ͻ�20��");
				TextOut(hdc, 180, 170, temp, strlen(temp));
	
			TextOut(hdc, 200, 240, "��  ��: Ctrl", 12);
			TextOut(hdc, 200, 290, "��  ʼ: Enter", 13);
			TextOut(hdc, 200, 340, "��  ��: Esc", 11);

			lpDDSBack->ReleaseDC(hdc);

		GetRect(&rect, 135, 150, 50, 50);
		rval = lpDDSBack->Blt( &rect, lpDDSPlane[1], NULL, DDBLT_WAIT|DDBLT_KEYSRC , NULL);
			if(rval == DDERR_SURFACELOST)    lpDDSBack->Restore();
		GetRect(&rect, 292, 150, 50, 50);
		rval = lpDDSBack->Blt( &rect, lpDDSPlane[1], NULL, DDBLT_WAIT|DDBLT_KEYSRC , NULL);
			if(rval == DDERR_SURFACELOST)    lpDDSBack->Restore();
	}

	if (gamestatus == GAME_START)//��Ϸ������
	{
		for (int i = 0; i<planeremain; i++)//�����һ�
		{
			GetRect(&rect, backrect.left+10+(i*25), backrect.bottom-30,20,20);
			rval = lpDDSBack->Blt( &rect, lpDDSPlane[1], NULL, DDBLT_WAIT|DDBLT_KEYSRC , NULL);
			if(rval == DDERR_SURFACELOST)    lpDDSBack->Restore();
		}

		//���Ƶл�
		GetRect(&rect, backrect.right-60, backrect.bottom-30,20,20);
		rval = lpDDSBack->Blt( &rect, lpDDSEnemy, NULL, DDBLT_WAIT|DDBLT_KEYSRC , NULL);
		if(rval == DDERR_SURFACELOST)    lpDDSBack->Restore();

		//�ܷ�
		lpDDSBack->GetDC(&hdc);
			SetBkMode(hdc, TRANSPARENT);
			SetTextColor(hdc, RGB(255,0,0)); 
			//�÷�
			sprintf(temp, "���ʱ��:%6d", timeSurvive);
			SetTextColor(hdc,RGB(0,255,0));
			TextOut(hdc, backwidth/2-40, 70, temp, strlen(temp));
			//����л���
			sprintf(temp, "x%d", enemylost);
			TextOut(hdc, backrect.right-35, backrect.bottom-25, temp, strlen(temp));			
		lpDDSBack->ReleaseDC(hdc);
	}

	if (gamestatus == GAME_OVER)//��Ϸ����
	{
		lpDDSBack->GetDC(&hdc);
		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, RGB(0,255,0)); 
			sprintf(temp, "����� %d ��", timeSurvive);
			TextOut(hdc, backwidth/2-40, backheight/2-80, "Game Over", 9);
			TextOut(hdc, backwidth/2-55, backheight/2-30, temp, strlen(temp));
			
			if (timeSurvive >= 20)
			{
				TextOut(hdc, backwidth/2-50, backheight/2, "�ɹ�����ս����", 14);
				ofstream fout("g_20s.dat");
				fout << "finished" << endl;
				fout.close();
			}
			else TextOut(hdc, backwidth/2-50, backheight/2, "���س�������", 12);
		lpDDSBack->ReleaseDC(hdc);
	}
}

//****************************
//������Ļ
void UpdateFrame(void )
{
	//��ⷢ���ӵ�
	if(GetAsyncKeyState(VK_CONTROL) && gamestatus == GAME_START && cancontrol == TRUE )
	{
		static int newtime,oldtime = GetTickCount();
		newtime = GetTickCount();
		if (newtime>oldtime+100)
		{
			oldtime = newtime;
			buletposleft[bulet].x = pos.x+13;
			buletposleft[bulet].y = pos.y;
			buletposright[bulet].x = pos.x+32;
			buletposright[bulet].y = pos.y;
			bulet++;
			if (bulet == BULETNUMBER) bulet = 0;
		}
	}

	//�ں�̨ҳ���ϵ���һϵ�л��ƺ�����ɻ�ͼ����
	DrawMap();
	//��ɫ���
	/*DDBLTFX ddbltfx; 
	ZeroMemory( &ddbltfx, sizeof(ddbltfx));
	ddbltfx.dwSize = sizeof(ddbltfx);
	ddbltfx.dwFillColor = DDColorMatch(lpDDSBack, RGB(255,0,0)) ;	// 
	lpDDSBack->Blt(
		NULL,        // Ŀ�����
		NULL, NULL,  // Դҳ���Դ����
		DDBLT_COLORFILL, &ddbltfx);
	*/
	CheckHit();
	DrawEnemy();
	DrawBulet();
	DrawPlane();
	DrawFlame();
	DrawText();
	//��󽫺�̨ҳ�滻��ǰ̨
	Flip();
}

//********************************
//�ı䱳����ͼ
void ChangeMap(void)
{
	static int b = 0;
	if (b == 0)
	{
		DDReLoadBitmap(lpDDSMap, "map2.bmp");
		b = 1;
	}
	else if (b == 1)
	{
		DDReLoadBitmap(lpDDSMap, "map3.bmp");
		b = 2;
	}
	else if (b == 2)
	{
		DDReLoadBitmap(lpDDSMap, "map1.bmp");
		b = 0;
	}
}



//******************************************
//�����ӵ�
void DrawBulet(void)
{
	for (int i = 0; i<BULETNUMBER; i++)
	{
		RECT rectleft, rectright;
		rectleft.left = buletposleft[i].x;
		rectleft.top = buletposleft[i].y;
		rectleft.right = rectleft.left+5;
		rectleft.bottom = rectleft.top+13;
		rectright.left = buletposright[i].x;
		rectright.top = buletposright[i].y;
		rectright.right = rectright.left+5;
		rectright.bottom = rectright.top+13;
	
		HRESULT rval;
		rval = lpDDSBack->Blt( &rectleft, lpDDSBulet[0], NULL, DDBLT_WAIT|DDBLT_KEYSRC , NULL);
			if(rval == DDERR_SURFACELOST)    lpDDSBack->Restore();
		rval = lpDDSBack->Blt( &rectright, lpDDSBulet[1], NULL, DDBLT_WAIT|DDBLT_KEYSRC , NULL);
			if(rval == DDERR_SURFACELOST)    lpDDSBack->Restore();

		buletposleft[i].x--;
		buletposleft[i].y -= 12;
		buletposright[i].x++;
		buletposright[i].y -= 12;
	}
}

//*************************
//���Ƶл�
void DrawEnemy(void)
{
	RECT rect;
	HRESULT rval;

	for (int i = 0; i<ENEMYNUMBER; i++)
	{
		if (enemydead[i] != 0)//����л�δ��
		{
			switch (enemystyle[i])
			{
			case 0://�л�0
				GetRect( &rect, enemypos[i].x, enemypos[i].y, 35, 35);
				rval = lpDDSBack->Blt( &rect, lpDDSEnemy, NULL, DDBLT_WAIT|DDBLT_KEYSRC , NULL);
				if(rval == DDERR_SURFACELOST)    lpDDSBack->Restore();
	
				enemypos[i].y += enemyspeed[i]+(movespeed-2);
				enemypos[i].x += enemydir[i];
				break;
			case 1://�ӵ�
				GetRect( &rect, enemypos[i].x, enemypos[i].y, 8, 8);
				rval = lpDDSBack->Blt( &rect, lpDDSBall, NULL, DDBLT_WAIT|DDBLT_KEYSRC , NULL);
				if(rval == DDERR_SURFACELOST)    lpDDSBack->Restore();
	
				enemypos[i].y += enemyspeed[i]+(movespeed-2);
				enemypos[i].x += enemydir[i];
				break;
			}
		}

		if (enemypos[i].y>backrect.bottom || enemydead[i] == 0)
		{
			enemypos[i].x = (int)(forewidth*((long)rand()/32767.0)) +35;
			enemypos[i].y = (int)(35*((long)rand()/32767.0));
			enemyspeed[i] = (int)(4*((long)rand()/32767.0))+1;
			enemydir[i] = (int)(3*((long)rand()/32767.0))-1;//-1,0,1
			
			//�л�����
			enemystyle[i] = (int)(2*((long)rand()/32767.0));//0,1
			enemydead[i] = 1;//���˸���
		}
	}
}

//******************************
//��ײ���
void CheckHit(void)
{
	if (gamestatus != GAME_START) return;//�����Ϸ����

	//���л��Ƿ񱻻���
	for (int e = 0; e < ENEMYNUMBER; e++)
	{
		if (enemydead[e] == 0) continue;//����������
		switch (enemystyle[e])
		{
		case 1://���ӵ�
			break;
		case 0://�ǵл�
			if (OutOfRange(enemypos[e])) continue;//�л����ڴ�����
			for (int b = 0; b < BULETNUMBER; b++)
			{
				//������ӵ�
				if (!OutOfRange(buletposleft[b]))
				{
					if (abs(enemypos[e].x - buletposleft[b].x + 15) < 10 && abs(enemypos[e].y - buletposleft[b].y + 11) < 10)
					{
						enemydead[e] = 0;//ʹ�л�����
						buletposleft[b].y = 0;//ʹ�ӵ���ʧ
						flamepos[e].x = enemypos[e].x-10;//��������
						flamepos[e].y = enemypos[e].y-8;//��������
						flamestatus[e] = 1;//����״̬
						enemylost++;
						MessageBeep(0);
					}
				}
				//������ӵ�
				if (!OutOfRange(buletposright[b]))
				{
					if (abs(enemypos[e].x-buletposright[b].x+15)<10 && abs(enemypos[e].y-buletposright[b].y+11)<10)
					{
						enemydead[e] = 0;//ʹ�л�����
						buletposright[b].y = 0;//ʹ�ӵ���ʧ
						flamepos[e].x = enemypos[e].x-10;//��������
						flamepos[e].y = enemypos[e].y-8;//��������
						flamestatus[e] = 1;//����״̬
						enemylost++;
						MessageBeep(0);
					}
				}
				//�ǵл���
			}
		}
	}

	//����һ��Ƿ�ײ��
	if (super == FALSE)
	{
		for (int p = 0; p<ENEMYNUMBER; p++)
		{
			if(enemydead[p] ==0) continue;
			if (OutOfRange(enemypos[p])) continue;
			switch (enemystyle[p])
			{
			case 1://�ӵ�
				if (abs(pos.x-enemypos[p].x+21)<15 && abs(pos.y-enemypos[p].y+26)<12)
				{
					enemydead[p] = 0;
					deadflamepos.x = enemypos[p].x-29;
					deadflamepos.y = enemypos[p].y-29;
					deadflamestatus = 1;
					planeremain--;
					MessageBeep(0);
				}
				break;
			case 0://�л�
				if (abs(pos.x-enemypos[p].x+7)<15 && abs(pos.y-enemypos[p].y+12)<12)
				{
					enemydead[p] = 0;
					deadflamepos.x = enemypos[p].x-15;
					deadflamepos.y = enemypos[p].y-15;
					deadflamestatus = 1;
					planeremain--;
					MessageBeep(0);
				}
				break;
			}
		}
	}
	
	if (planeremain == -1) //��Ϸ����
	{
		gamestatus = GAME_OVER;
		KillTimer(hWnd, TIMER);
	}
}

//*************************
//��������Ƿ񳬳��߽�
BOOL OutOfRange(POINT p)
{
	if (p.x<35 || p.x>WINDOW_WIDTH+35 || p.y<35 || p.y>WINDOW_HEIGHT+35)
		return TRUE;
	else
		return FALSE;
}

//******************
//���һ��RECT�ṹ
void GetRect(RECT* rect, long left, long top, long width, long height)
{
	rect->left = left;
	rect->top = top;
	rect->right = left+width;
	rect->bottom = top+height;
	return;
}

//***************************************
//���Ʊ�ը����
void DrawFlame(void )
{
	//���Ƶл�����
	for (int f = 0; f<ENEMYNUMBER; f++)
	{
		if (flamestatus[f] != 0)
		{
			RECT rect,srect;
			GetRect(&rect, flamepos[f].x, flamepos[f].y, 54, 50);
			GetRect(&srect, (flamestatus[f]-1)*54, 0, 54, 50);

			HRESULT rval;//���ƻ���
			rval = lpDDSBack->Blt( &rect, lpDDSFlame, &srect, DDBLT_WAIT|DDBLT_KEYSRC , NULL);
			if(rval == DDERR_SURFACELOST)    lpDDSBack->Restore();
			
			static int otime[ENEMYNUMBER];
			if (GetTickCount()-otime[f]>150)
			{
				otime[f] = GetTickCount();
				flamestatus[f]++;//���»���״̬
				if (flamestatus[f] == 11 ) flamestatus[f] = 0;//�������
			}

		}
	}

	//�����һ�ײ�ٻ���
	if (deadflamestatus != 0)
	{
		RECT rect, srect;
		GetRect(&rect, deadflamepos.x, deadflamepos.y, 66, 66);
		GetRect(&srect,(deadflamestatus-1)*66, 0, 66, 66);

		HRESULT rval;//���ƻ���
		rval = lpDDSBack->Blt( &rect, lpDDSDead, &srect, DDBLT_WAIT|DDBLT_KEYSRC , NULL);
		if(rval == DDERR_SURFACELOST)    lpDDSBack->Restore();
			
		static int oldtime = 0;//��ʱ
		if (GetTickCount()-oldtime>200)
		{
			oldtime = GetTickCount();
			deadflamestatus++;//���»���״̬
			if (deadflamestatus == 9 ) deadflamestatus = 0;//�������
		}
	}
}