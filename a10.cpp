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

#define WINDOW_WIDTH	350//窗口宽度
#define WINDOW_HEIGHT	450//窗口高度
#define TIMER 1 //定时器ID

LPDIRECTDRAW            lpDD;       //DirectDraw对象
LPDIRECTDRAWCLIPPER		lpClipper;	//DirectDrawClipper对象
LPDIRECTDRAWSURFACE     lpDDSFront; //以下均为DirectDrawSurface对象
LPDIRECTDRAWSURFACE     lpDDSBack;   
LPDIRECTDRAWSURFACE     lpDDSPlane[3];
LPDIRECTDRAWSURFACE     lpDDSBall;
LPDIRECTDRAWSURFACE     lpDDSMap;   
LPDIRECTDRAWSURFACE     lpDDSBulet[2];
LPDIRECTDRAWSURFACE     lpDDSFlame;
LPDIRECTDRAWSURFACE     lpDDSDead;
LPDIRECTDRAWSURFACE     lpDDSEnemy;   

const int BULETNUMBER=10;	//子弹总数
const int ENEMYNUMBER=16;	//敌机总数

HWND	hWnd;				//主窗口句柄
BOOL	bActive = TRUE;		//应用程序是否活跃
char	apppath[50];		//应用程序路径

int		speed = 12;			//延时速度
int		movespeed = 2;		//移动速度

int		gamestatus;			//游戏进程
#define GAME_LOG	0
#define GAME_START	1
#define GAME_OVER	2

BOOL	super = FALSE;		//我机是否无敌
BOOL	cancontrol = FALSE;	//我机是否能控制

int		Plane = 1;			//我机姿态
POINT	pos;				//我机位置
int		forewidth, foreheight;//前台页面的宽度和高度
RECT	backrect;			//后台页面映射矩形
int		backwidth, backheight;//后台页面的宽度和高度
int		enemylost;			//敌机损失数量
int		planeremain;		//我机剩余数量

POINT	buletposleft[BULETNUMBER];//左侧子弹位置
POINT	buletposright[BULETNUMBER];//右侧子弹位置
int		bulet=0;					//当前子弹记录

POINT	enemypos[ENEMYNUMBER];	//敌机位置
int		enemyspeed[ENEMYNUMBER];//敌机速度
int		enemydir[ENEMYNUMBER];	//敌机方向
int		enemystyle[ENEMYNUMBER];//敌机是子弹还是飞机
int		enemydead[ENEMYNUMBER]; //敌机是否已死

POINT	flamepos[ENEMYNUMBER];	//火焰位置
int		flamestatus[ENEMYNUMBER];//火焰状态记录

POINT	deadflamepos;			//我机死时的火焰位置
int		deadflamestatus=0;		//火焰状态记录
int		timeSurvive;

//函数声明
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
//应用程序入口
int PASCAL WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
                        LPSTR lpCmdLine, int nCmdShow)
{
	MSG		msg;

	//初始化主窗口
	hWnd=InitWindow( hInstance, nCmdShow ) ;
	if (hWnd == NULL )
		return FALSE;
	
	//初始化DirectDraw环境，并实现DirectDraw功能
	if (!InitDDraw())
	{
		MessageBox(GetActiveWindow(), "初始化DirectDraw过程中出错！请检查你是否正确的安装了DirectX。", "Error", MB_OK );
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
			//计算帧频率
			static int NewCount, OldCount=0;
			NewCount = GetTickCount(); 
			if (NewCount > OldCount+speed)
			{
				OldCount=NewCount;
				UpdateFrame();//更新画面
			}
		}
		//等待消息
		else WaitMessage();
	}
}

//******************************************************************
//创建主窗口。
HWND InitWindow( HINSTANCE hInstance, int nCmdShow )
{
    HWND				hwnd;	
    WNDCLASS			wc;		

	//填充窗口类结构
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

	//注册窗口类
    RegisterClass( &wc );
    
	//创建主窗口
    hwnd = CreateWindowEx(
		0,
		"Survive",
		"是女生就活20秒",
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

	//显示并更新窗口
    ShowWindow( hwnd, nCmdShow );
    UpdateWindow( hwnd );

	return hwnd;
}

//*****************************
//处理主窗口消息
LRESULT CALLBACK WinProc( HWND hWnd, UINT message, 
                            WPARAM wParam, LPARAM lParam )
{
    switch( message )
    {
	case WM_ACTIVATEAPP://
		bActive = wParam;
		break;
    case WM_KEYDOWN://击键消息
        switch( wParam )
        {
        case VK_ESCAPE://退出
            PostMessage(hWnd, WM_CLOSE, 0, 0);
            break;
		case VK_RETURN :
			if (gamestatus!= GAME_START)//开始游戏，进行初始化
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
	case WM_TIMER://定时器消息
		switch (wParam)
		{
		case TIMER:
			timeSurvive++;
			break;
		}
		break;
    case WM_DESTROY://退出消息
        FreeDDraw();//释放所有DirectDraw对象
		//关闭midi				
        PostQuitMessage( 0 );
        break;
    }
	
	//调用缺省消息处理过程
    return DefWindowProc(hWnd, message, wParam, lParam);
}

//******************************************************************
//初始化DirectDraw环境
BOOL InitDDraw(void)
{
	DWORD dwFlags;
	DDSURFACEDESC ddsd;

	//创建DirectDraw对象
	DirectDrawCreate(NULL, &lpDD, NULL);

	//设置协作级别为窗口模式
	dwFlags = DDSCL_NORMAL;
	lpDD->SetCooperativeLevel(hWnd, dwFlags);

	//创建主页面
	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	lpDD->CreateSurface(&ddsd, &lpDDSFront, NULL);

	//创建离屏页面
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

	//创建地图页面
	ddsd.dwWidth = 64;
	ddsd.dwHeight = 64;
	lpDD->CreateSurface( &ddsd, &lpDDSMap, NULL );
	DDReLoadBitmap(lpDDSMap, "map1.bmp");

	//创建飞机页面
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
	
	
	//创建敌机
	ddsd.dwWidth = 35;
	ddsd.dwHeight = 35;
	lpDD->CreateSurface( &ddsd, &lpDDSEnemy, NULL );
	DDReLoadBitmap(lpDDSEnemy, "enemy.bmp");
	DDSetColorKey(lpDDSEnemy,RGB(0,0,0));

	//创建火焰页面
	ddsd.dwWidth = 540;
	ddsd.dwHeight = 50;
	lpDD->CreateSurface( &ddsd, &lpDDSFlame, NULL );
	DDReLoadBitmap(lpDDSFlame, "flame.bmp");
	DDSetColorKey(lpDDSFlame,RGB(0,0,0));

	//创建我机撞毁火焰页面
	ddsd.dwWidth = 528;
	ddsd.dwHeight = 66;
	lpDD->CreateSurface( &ddsd, &lpDDSDead, NULL );
	DDReLoadBitmap(lpDDSDead, "flamedead.bmp");
	DDSetColorKey(lpDDSDead,RGB(0,0,0));

	//创建敌机子弹页面
	ddsd.dwWidth = 8;
	ddsd.dwHeight = 8;
	lpDD->CreateSurface( &ddsd, &lpDDSBall, NULL );
	DDReLoadBitmap(lpDDSBall, "ball.bmp");
	DDSetColorKey(lpDDSBall,RGB(0,0,0));

	//创建我机子弹页面
	ddsd.dwWidth = 5;
	ddsd.dwHeight = 13;
	lpDD->CreateSurface( &ddsd, &lpDDSBulet[0], NULL );
	lpDD->CreateSurface( &ddsd, &lpDDSBulet[1], NULL );

	DDReLoadBitmap(lpDDSBulet[0], "bulet0.bmp");
	DDReLoadBitmap(lpDDSBulet[1], "bulet1.bmp");

	DDSetColorKey(lpDDSBulet[0],RGB(0,0,0));
	DDSetColorKey(lpDDSBulet[1],RGB(0,0,0));

	//创建Clipper裁剪器
	lpDD->CreateClipper(0, &lpClipper, NULL);
	lpClipper->SetHWnd(0, hWnd);
	lpDDSFront->SetClipper(lpClipper);

	//开始游戏初始化
	for (int k = 0; k < ENEMYNUMBER; k++) 
		enemydead[k] = 0;
	enemylost = 0;
	planeremain = 0;
	gamestatus = GAME_LOG;
	//设置定时器
	SetTimer(hWnd, TIMER, 1000, NULL);

	return TRUE;
} 

//*****************************************************
//释放所有的DirectDraw对象。
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
//换页
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
//绘制我机
void DrawPlane( void )
{
	if (gamestatus != GAME_START) return;//游戏结束

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
		if (GetTickCount()>oldtime+3000)//无敌时间3秒
			super = FALSE;

	if (cancontrol == FALSE)
	{
		if (pos.y>backrect.bottom-100) //飞机刚出现时是无法控制的
		{
			pos.y -= movespeed;
			Plane = 1;
		}
		else
			cancontrol = TRUE;
	}
	else
	{
		//检测方向键
		Plane = 1;
		if(GetAsyncKeyState(VK_UP))		{pos.y = pos.y - movespeed;}
		if(GetAsyncKeyState(VK_DOWN))	{pos.y = pos.y+movespeed;}
		if(GetAsyncKeyState(VK_LEFT))	{pos.x = pos.x-movespeed;Plane = 0;}
		if(GetAsyncKeyState(VK_RIGHT))	{pos.x = pos.x+movespeed;Plane = 2;}
	
		//范围检查
		if (pos.x<65)	pos.x = 65;
		if (pos.x>backwidth-65-50)	pos.x = backwidth - 65 - 50;
		if (pos.y<65)	pos.y = 65;
		if (pos.y>backheight - 65 - 60)	pos.y = backheight - 65 - 60;
	}

	if (super == TRUE) //无敌时的闪烁效果
	{
		static int ot = 0;//延时
		if (GetTickCount() - ot > 30)
			ot = GetTickCount();
		else
			return;
	}

	//绘制飞机
	RECT rect;
	GetRect(&rect, pos.x, pos.y, 50, 60);
	HRESULT rval = lpDDSBack->Blt( &rect, lpDDSPlane[Plane], NULL, DDBLT_WAIT|DDBLT_KEYSRC , NULL);
	if(rval == DDERR_SURFACELOST)    lpDDSBack->Restore();
}

//***************************************
//绘制背景地图
void DrawMap(void )
{
	//背景贴图的偏移坐标
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
//绘制说明文字及图象
void DrawText(void )
{
	HRESULT rval;
	RECT rect;
	HDC hdc;
	char temp[100];
	
	if (gamestatus == GAME_LOG)//初始画面
	{
		lpDDSBack->GetDC(&hdc);
		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, RGB(0,255,0)); 
			sprintf(temp, "是女生就活20秒");
				TextOut(hdc, 180, 170, temp, strlen(temp));
	
			TextOut(hdc, 200, 240, "子  弹: Ctrl", 12);
			TextOut(hdc, 200, 290, "开  始: Enter", 13);
			TextOut(hdc, 200, 340, "结  束: Esc", 11);

			lpDDSBack->ReleaseDC(hdc);

		GetRect(&rect, 135, 150, 50, 50);
		rval = lpDDSBack->Blt( &rect, lpDDSPlane[1], NULL, DDBLT_WAIT|DDBLT_KEYSRC , NULL);
			if(rval == DDERR_SURFACELOST)    lpDDSBack->Restore();
		GetRect(&rect, 292, 150, 50, 50);
		rval = lpDDSBack->Blt( &rect, lpDDSPlane[1], NULL, DDBLT_WAIT|DDBLT_KEYSRC , NULL);
			if(rval == DDERR_SURFACELOST)    lpDDSBack->Restore();
	}

	if (gamestatus == GAME_START)//游戏进行中
	{
		for (int i = 0; i<planeremain; i++)//绘制我机
		{
			GetRect(&rect, backrect.left+10+(i*25), backrect.bottom-30,20,20);
			rval = lpDDSBack->Blt( &rect, lpDDSPlane[1], NULL, DDBLT_WAIT|DDBLT_KEYSRC , NULL);
			if(rval == DDERR_SURFACELOST)    lpDDSBack->Restore();
		}

		//绘制敌机
		GetRect(&rect, backrect.right-60, backrect.bottom-30,20,20);
		rval = lpDDSBack->Blt( &rect, lpDDSEnemy, NULL, DDBLT_WAIT|DDBLT_KEYSRC , NULL);
		if(rval == DDERR_SURFACELOST)    lpDDSBack->Restore();

		//总分
		lpDDSBack->GetDC(&hdc);
			SetBkMode(hdc, TRANSPARENT);
			SetTextColor(hdc, RGB(255,0,0)); 
			//得分
			sprintf(temp, "存活时间:%6d", timeSurvive);
			SetTextColor(hdc,RGB(0,255,0));
			TextOut(hdc, backwidth/2-40, 70, temp, strlen(temp));
			//击落敌机数
			sprintf(temp, "x%d", enemylost);
			TextOut(hdc, backrect.right-35, backrect.bottom-25, temp, strlen(temp));			
		lpDDSBack->ReleaseDC(hdc);
	}

	if (gamestatus == GAME_OVER)//游戏结束
	{
		lpDDSBack->GetDC(&hdc);
		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, RGB(0,255,0)); 
			sprintf(temp, "共存活 %d 秒", timeSurvive);
			TextOut(hdc, backwidth/2-40, backheight/2-80, "Game Over", 9);
			TextOut(hdc, backwidth/2-55, backheight/2-30, temp, strlen(temp));
			
			if (timeSurvive >= 20)
			{
				TextOut(hdc, backwidth/2-50, backheight/2, "成功结束战斗！", 14);
				ofstream fout("g_20s.dat");
				fout << "finished" << endl;
				fout.close();
			}
			else TextOut(hdc, backwidth/2-50, backheight/2, "按回车键继续", 12);
		lpDDSBack->ReleaseDC(hdc);
	}
}

//****************************
//更新屏幕
void UpdateFrame(void )
{
	//检测发射子弹
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

	//在后台页面上调用一系列绘制函数完成绘图工作
	DrawMap();
	//单色填充
	/*DDBLTFX ddbltfx; 
	ZeroMemory( &ddbltfx, sizeof(ddbltfx));
	ddbltfx.dwSize = sizeof(ddbltfx);
	ddbltfx.dwFillColor = DDColorMatch(lpDDSBack, RGB(255,0,0)) ;	// 
	lpDDSBack->Blt(
		NULL,        // 目标矩形
		NULL, NULL,  // 源页面和源矩形
		DDBLT_COLORFILL, &ddbltfx);
	*/
	CheckHit();
	DrawEnemy();
	DrawBulet();
	DrawPlane();
	DrawFlame();
	DrawText();
	//最后将后台页面换至前台
	Flip();
}

//********************************
//改变背景地图
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
//绘制子弹
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
//绘制敌机
void DrawEnemy(void)
{
	RECT rect;
	HRESULT rval;

	for (int i = 0; i<ENEMYNUMBER; i++)
	{
		if (enemydead[i] != 0)//如果敌机未毁
		{
			switch (enemystyle[i])
			{
			case 0://敌机0
				GetRect( &rect, enemypos[i].x, enemypos[i].y, 35, 35);
				rval = lpDDSBack->Blt( &rect, lpDDSEnemy, NULL, DDBLT_WAIT|DDBLT_KEYSRC , NULL);
				if(rval == DDERR_SURFACELOST)    lpDDSBack->Restore();
	
				enemypos[i].y += enemyspeed[i]+(movespeed-2);
				enemypos[i].x += enemydir[i];
				break;
			case 1://子弹
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
			
			//敌机类型
			enemystyle[i] = (int)(2*((long)rand()/32767.0));//0,1
			enemydead[i] = 1;//敌人复活
		}
	}
}

//******************************
//碰撞检测
void CheckHit(void)
{
	if (gamestatus != GAME_START) return;//如果游戏结束

	//检测敌机是否被击落
	for (int e = 0; e < ENEMYNUMBER; e++)
	{
		if (enemydead[e] == 0) continue;//敌人已消灭
		switch (enemystyle[e])
		{
		case 1://是子弹
			break;
		case 0://是敌机
			if (OutOfRange(enemypos[e])) continue;//敌机不在窗口内
			for (int b = 0; b < BULETNUMBER; b++)
			{
				//检测左子弹
				if (!OutOfRange(buletposleft[b]))
				{
					if (abs(enemypos[e].x - buletposleft[b].x + 15) < 10 && abs(enemypos[e].y - buletposleft[b].y + 11) < 10)
					{
						enemydead[e] = 0;//使敌机销毁
						buletposleft[b].y = 0;//使子弹消失
						flamepos[e].x = enemypos[e].x-10;//火焰坐标
						flamepos[e].y = enemypos[e].y-8;//火焰坐标
						flamestatus[e] = 1;//火焰状态
						enemylost++;
						MessageBeep(0);
					}
				}
				//检测右子弹
				if (!OutOfRange(buletposright[b]))
				{
					if (abs(enemypos[e].x-buletposright[b].x+15)<10 && abs(enemypos[e].y-buletposright[b].y+11)<10)
					{
						enemydead[e] = 0;//使敌机销毁
						buletposright[b].y = 0;//使子弹消失
						flamepos[e].x = enemypos[e].x-10;//火焰坐标
						flamepos[e].y = enemypos[e].y-8;//火焰坐标
						flamestatus[e] = 1;//火焰状态
						enemylost++;
						MessageBeep(0);
					}
				}
				//是敌机完
			}
		}
	}

	//检测我机是否撞毁
	if (super == FALSE)
	{
		for (int p = 0; p<ENEMYNUMBER; p++)
		{
			if(enemydead[p] ==0) continue;
			if (OutOfRange(enemypos[p])) continue;
			switch (enemystyle[p])
			{
			case 1://子弹
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
			case 0://敌机
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
	
	if (planeremain == -1) //游戏结束
	{
		gamestatus = GAME_OVER;
		KillTimer(hWnd, TIMER);
	}
}

//*************************
//检测物体是否超出边界
BOOL OutOfRange(POINT p)
{
	if (p.x<35 || p.x>WINDOW_WIDTH+35 || p.y<35 || p.y>WINDOW_HEIGHT+35)
		return TRUE;
	else
		return FALSE;
}

//******************
//填充一个RECT结构
void GetRect(RECT* rect, long left, long top, long width, long height)
{
	rect->left = left;
	rect->top = top;
	rect->right = left+width;
	rect->bottom = top+height;
	return;
}

//***************************************
//绘制爆炸火焰
void DrawFlame(void )
{
	//绘制敌机火焰
	for (int f = 0; f<ENEMYNUMBER; f++)
	{
		if (flamestatus[f] != 0)
		{
			RECT rect,srect;
			GetRect(&rect, flamepos[f].x, flamepos[f].y, 54, 50);
			GetRect(&srect, (flamestatus[f]-1)*54, 0, 54, 50);

			HRESULT rval;//绘制火焰
			rval = lpDDSBack->Blt( &rect, lpDDSFlame, &srect, DDBLT_WAIT|DDBLT_KEYSRC , NULL);
			if(rval == DDERR_SURFACELOST)    lpDDSBack->Restore();
			
			static int otime[ENEMYNUMBER];
			if (GetTickCount()-otime[f]>150)
			{
				otime[f] = GetTickCount();
				flamestatus[f]++;//更新火焰状态
				if (flamestatus[f] == 11 ) flamestatus[f] = 0;//火焰完毕
			}

		}
	}

	//绘制我机撞毁火焰
	if (deadflamestatus != 0)
	{
		RECT rect, srect;
		GetRect(&rect, deadflamepos.x, deadflamepos.y, 66, 66);
		GetRect(&srect,(deadflamestatus-1)*66, 0, 66, 66);

		HRESULT rval;//绘制火焰
		rval = lpDDSBack->Blt( &rect, lpDDSDead, &srect, DDBLT_WAIT|DDBLT_KEYSRC , NULL);
		if(rval == DDERR_SURFACELOST)    lpDDSBack->Restore();
			
		static int oldtime = 0;//延时
		if (GetTickCount()-oldtime>200)
		{
			oldtime = GetTickCount();
			deadflamestatus++;//更新火焰状态
			if (deadflamestatus == 9 ) deadflamestatus = 0;//火焰完毕
		}
	}
}