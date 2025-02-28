


#define DEBUG 0
//#define NDEBUG
//#define WINVER 0x0A00

#include <bits/stdc++.h>
#include <windows.h>
#include <gdiplus.h>
//#include <conio.h>
using namespace std;



namespace utils {

const int dx4[]={1,0,-1,0,0};
const int dy4[]={0,-1,0,1,0};
const int dir24[][4]={
	{0,1,2,3},{0,1,3,2},{0,2,1,3},{0,2,3,1},{0,3,1,2},{0,3,2,1},
	{1,0,2,3},{1,0,3,2},{1,2,0,3},{1,2,3,0},{1,3,0,2},{1,3,2,0},
	{2,0,1,3},{2,0,3,1},{2,1,0,3},{2,1,3,0},{2,3,0,1},{2,3,1,0},
	{3,0,1,2},{3,0,2,1},{3,1,0,2},{3,1,2,0},{3,2,0,1},{3,2,1,0},
};
const int (&dir4)[4]=dir24[0];
bool inBoard(int x,int y,int xMAX,int yMAX) {
	return 0<=x && x<xMAX && 0<=y && y<yMAX;
}
bool inBoard(int x,int y,int xMIN,int yMIN,int xMAX,int yMAX) {
	return xMIN<=x && x<=xMAX && yMIN<=y && y<=yMAX;
}
//
void msDelayOn() {timeBeginPeriod(1);}
void msDelayOff() {timeEndPeriod(1);}
void msDelayTest() {
	LARGE_INTEGER tmp1,tmp2,frequence;
	QueryPerformanceFrequency(&frequence);
	for(int i=0; i<3; ++i) {
		QueryPerformanceCounter(&tmp1);
		Sleep(10);
		QueryPerformanceCounter(&tmp2);
		cout<<1e3*(tmp2.QuadPart-tmp1.QuadPart)/frequence.QuadPart<<"   ";
	}
	getchar();
}
#if DEBUG
#define Assert(expr) if(!(expr)) MessageBox(NULL,\
	(to_string(__LINE__)+"line: "+#expr).c_str(),"assertion failed",MB_OK)
#else
#define Assert(expr) void(expr)
#endif
//gdi+
ULONG_PTR gdipToken;
Gdiplus::GdiplusStartupInput gdipInput;
void gdipOn() {Gdiplus::GdiplusStartup(&gdipToken,&gdipInput,NULL);}
void gdipOff() {Gdiplus::GdiplusShutdown(gdipToken);}
//
struct bitTable {
	vector<int> mask;
	bitTable() {}
	bitTable(int size) {
		mask.resize((size+31)>>5);
	}
	void set(int pos) {
		mask[pos>>5] |= 1<<(pos&31);
	}
	void reset(int pos) {
		mask[pos>>5] &= ~(1<<(pos&31));
	}
	bool test(int pos) {
		return mask[pos>>5] & 1<<(pos&31);
	}
	void resize(int size) {
		mask.resize(0);
		mask.resize((size+31)>>5,0);
	}
};
struct _dsu {
	vector<int> f,size;
	_dsu(int cnt) {
		f.resize(cnt);
		size.resize(cnt,1);
		iota(f.begin(),f.end(),0);
	}
	int top(int x) {
		if(f[x]==x)return x;
		return f[x]=top(f[x]);
	}
	void link(int a,int b) {
		if(size[top(a)]<size[top(b)])
			swap(a,b);
		size[f[a]]+=size[f[b]];
		f[f[b]]=f[f[a]];
	}
};

//窗口基类
template<class DerivedWindow> class BaseWindow {
public:
	static LRESULT CALLBACK
	msgForward(HWND hwnd, UINT uMsg, WPARAM wp, LPARAM lp) {
		DerivedWindow *pthis = NULL;
		if(uMsg == WM_NCCREATE) {
			CREATESTRUCT *cinfo = (CREATESTRUCT*)lp;
			pthis = (DerivedWindow*)cinfo->lpCreateParams;
			SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pthis);
			pthis->m_hwnd = hwnd;
		}
		else pthis = (DerivedWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		if(pthis) {
			return pthis->WndProc(hwnd, uMsg, wp, lp);
		}
		else return DefWindowProc(hwnd, uMsg, wp, lp);
	}
	BaseWindow():m_hwnd(NULL) {}
	HWND getHWnd() const {return m_hwnd;}
	void show(int nCmdShow) {ShowWindow(m_hwnd,nCmdShow);}
protected:
	void errorPause(const char *msg) {
		MessageBox(NULL,msg,"error",MB_OK);
	}
	virtual PCSTR className() const = 0;
	virtual LRESULT WndProc(HWND,UINT,WPARAM,LPARAM) = 0;
	HWND m_hwnd;
};
//绘图缓存
struct BufferContext {
    HDC hdcWindow=0;			// 窗口设备上下文
    HDC hdcMem=0;				// 内存设备上下文
    HBITMAP hbmBuffer=0;		// 双缓冲位图句柄
    HBITMAP hbmOld=0;			// 原位图句柄（用于恢复状态）
    int width=0,height=0;		// 绘图区尺寸
    uint32_t *pBuffer=0;		// 直接访问像素的指针
	Gdiplus::Graphics *gra=0;	// gdi+绘图上下文
	uint32_t &operator () (int x,int y) {
		return pBuffer[y*width+x];
	}
	void release() {
		SelectObject(hdcMem,hbmOld);
		DeleteObject(hbmBuffer);
		delete gra;
	}
	//在wm_create,wm_size消息中调用以更新缓存区
	void create(HWND hwnd,int w,int h) {
		//todo check wh
		//不必重新创建
		if(width==w && height==h) return;
		else width=w,height=h;
		//第一次创建
		if(!hbmBuffer) {
			hdcWindow=GetDC(hwnd);
			hdcMem=CreateCompatibleDC(hdcWindow);
		}
		//释放旧资源
		else release();
		//
		BITMAPINFO bmi={};
		bmi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth=w;
		bmi.bmiHeader.biHeight=-h;  // 顶向位图
		bmi.bmiHeader.biPlanes=1;
		bmi.bmiHeader.biBitCount=32;
		bmi.bmiHeader.biCompression=BI_RGB;
		//创建可直接访问的位图
		hbmBuffer=CreateDIBSection(
			hdcMem,&bmi,DIB_RGB_COLORS,(void**)&pBuffer,NULL,0);
		//绑定到内存DC
		hbmOld=(HBITMAP)SelectObject(hdcMem,hbmBuffer);
		gra=Gdiplus::Graphics::FromHDC(hdcMem);
	}
};

}	//end namespace utils
using namespace utils;



namespace core {

int boardWidth,boardHeight,maxBooms;
int boardSize;
using PII=pair<int,int>;
ostream &operator << (ostream &out,const PII &pos) {
	return out<<"{"<<pos.first<<","<<pos.second<<"}";
}
bool inBoard(int x,int y) {
	return 0<=x && x<boardWidth && 0<=y && y<boardHeight;
}
bool inBoard(const PII &pos) {
	return inBoard(pos.first,pos.second);
}
int posZip(int x,int y) {
	return y*boardWidth+x;
}
int posZip(PII pos) {
	return posZip(pos.first,pos.second);
}
PII posUnzip(int p) {
	return PII(p/boardWidth,p%boardWidth);
}
int dx8[]={1,1,0,-1,-1,-1,0,1,0};
int dy8[]={0,-1,-1,-1,0,1,1,1,0};
vector<PII> surroundCoords(int x,int y) {
	vector<PII> res;
	for(int dir=0; dir<8; ++dir) {
		int tx=x+dx8[dir],ty=y+dy8[dir];
		if(inBoard(tx,ty))res.push_back(PII(tx,ty));
	}
	return res;
}
vector<PII> surroundCoords(PII pos) {
	return surroundCoords(pos.first,pos.second);
}
unsigned rngSeed=time(NULL);
static mt19937 rng(rngSeed);
//
enum gameModes {
	sweeperModeClassic=0,
	sweeperModeLogic,
	gameDiffcultyEasy=0,
	gameDiffcultyNormal,
	gameDiffcultyHard,
	gameDiffcultyCustom,
};
int sweeperMode=sweeperModeClassic;
int gameDiffculty=gameDiffcultyEasy;
int customBoardW=10,customBoardH=10;
double customBoomRatio=0.2;
enum gameTiles {
	tile_0,
	tile_1,
	tile_2,
	tile_3,
	tile_4,
	tile_5,
	tile_6,
	tile_7,
	tile_8,
	tile_boom,
	tile_mask,
	tile_flag,
	tile_empty,
	_tile_count,
};
enum emojiStates {
	emojiInit,
	emojiPending,
	emojiFail,
	emojiWin,
	_emojiCount,
};
struct Board {
	vector<int> _map;
	Board() {}
	Board(int size,int x=0) {_map.resize(size,x);}
	int size() const {return _map.size();}
	void resize(int size) {_map.resize(size);}
	int &operator () (int x,int y) {return _map[posZip(x,y)];}
	int operator () (int x,int y) const {return _map[posZip(x,y)];}
	int &operator () (PII pos) {return _map[posZip(pos)];}
	int operator () (PII pos) const {return _map[posZip(pos)];}
	vector<int>::iterator begin() {return _map.begin();}
	vector<int>::iterator end() {return _map.end();}
	int &operator [] (int i) {return _map[i];}
	int operator [] (int i) const {return _map[i];}
	void show() const {
		static const char tileChars[_tile_count+1]=" 12345678*/X.";
		cout<<string(32,'=')<<endl;
		for(int j=0; j<boardHeight; ++j) {
			for(int i=0; i<boardWidth; ++i)
				cout<<tileChars[(*this)(i,j)]<<" ";
			cout<<endl;
		}
	}
}boomMap,maskedMap;
int getNumber(const Board &board,int x,int y) {
	int cnt=0;
	for(PII neighbor:surroundCoords(x,y))
	if(boomMap(neighbor)==tile_boom)
		cnt++;
	return cnt;
}
void _getNumbers() {
	Board count(boomMap.size());
	for(int j=0; j<boardHeight; ++j)
	for(int i=0; i<boardWidth; ++i)
	if(boomMap(i,j)!=tile_boom)
		count(i,j)=getNumber(boomMap,i,j);
	for(int j=0; j<boardHeight; ++j)
	for(int i=0; i<boardWidth; ++i)
	if(boomMap(i,j)!=tile_boom)
		boomMap(i,j)=count(i,j);
}
//0:no_effect 1:change 2:boom
int openCell_core(int x,int y) {
	Assert(inBoard(x,y));
	int &tile=maskedMap(x,y);
	if(tile==tile_empty || (tile_0<=tile && tile<=tile_8))return 0;
	else {
		tile=boomMap(x,y);
		if(tile!=tile_boom)return 1;
		else return 2;
	}
}
//
void generateBooms() {
	for(auto &tile:boomMap)tile=tile_0;
	for(int i=0; i<maxBooms; ++i)boomMap[i]=tile_boom;
	shuffle(boomMap.begin(),boomMap.end(),rng);
	_getNumbers();
	for(auto &tile:maskedMap)tile=tile_mask;
}
bool generateBoomsGetInfo=false;
namespace generateBoomsLogic {
	using fl=double;
	enum objType {
		objNumberMax=8,
		objVoid,
		objEmpty,
		objBoom,
		objMask,
		objVarious,
	};
	inline bool isObjNumber(const int &obj) {
		return 0<=obj && obj<=objNumberMax;
	}
	
	class Grid {
		int log2W;	//ceil(log2(width+2*borderW))
		int wMask;	//2^log2W-1
	protected:
		std::vector<int> _grid;
		int width,height,borderW;
	public:
		Grid() {}
		virtual ~Grid() {}
		int w()		const { return width;	}
		int h()		const { return height;	}
		int exW()	const { return borderW;	}
		//
		int POS(int x,int y)			const	{ return y<<log2W|x;		}
		void XY(int pos,int &x,int &y)	const	{ y=pos>>log2W,x=pos&wMask;	}
		int objOn(int x,int y)			const	{ return _grid[POS(x,y)];	}
		void setObj(int x,int y,int v)			{ _grid[POS(x,y)]=v;		}
		//
		void fillBorder(int obj=0);
		void fillCenter(int obj=0);
		int _setGrid();
		int _setSize(int w,int h,int exW);
		int initGrid(int w,int h,int exW=0,int obj=0,int exObj=0);
	};
	void Grid::fillBorder(int obj) {
		int w2=width+borderW*2,h2=height+borderW*2;
		for(int j=0; j<borderW; ++j)
		for(int i=0; i<w2; ++i) {
			setObj(i,j,obj);
			setObj(i,h2-1-j,obj);
		}
		int h1=height+borderW;
		for(int i=0; i<borderW; ++i)
		for(int j=borderW; j<h1; ++j) {
			setObj(i,j,obj);
			setObj(w2-1-i,j,obj);
		}
	}
	void Grid::fillCenter(int obj) {
		int w1=width+borderW,h1=height+borderW;
		for(int j=borderW; j<h1; ++j)
		for(int i=borderW; i<w1; ++i)
			setObj(i,j,obj);
	}
	int Grid::_setGrid() {
		int idx=0,p2=1;
		while(p2<width+2*borderW) {
			idx++;
			p2<<=1;
		}
		log2W=idx,wMask=p2-1;
		_grid.resize(p2*(height+2*borderW));
		_grid.shrink_to_fit();
		return 0;
	}
	int Grid::_setSize(int w,int h,int exW) {
		if(	0<=exW && exW<10000
			&& 0<=w && w<100000000
			&& 0<=h && h<100000000
			&& (fl)(w+exW*2)*(h+exW*2)<1e8)
		{
			width=w,height=h,borderW=exW;
			return 0;
		}
		std::cerr<<"\nError: invalid grid size("<<w<<","<<h<<","<<exW<<").\n";
		return -1;
	}
	int Grid::initGrid(int w,int h,int exW,int obj,int exObj) {
		int ret=_setSize(w,h,exW);
		if(ret>=0)
			ret=_setGrid();
		if(ret>=0) {
			fillCenter(obj);
			fillBorder(exObj);
		}
		return ret;
	}
	
	//扫雷类使用说明:
	//首先按需要分别设置参数: setSize(w,h),setBoomRate(r1,r2)
	//然后调用newSweeper()函数生成一个无猜扫雷
	//或者使用newRawSweeper()生成一个有猜扫雷
	class Sweeper:public Grid {
	
	public:
		//basic
		int size() const;
		int& grid(int p);
		int& grid(int x,int y);
		bool inGrid(int x,int y);
		void setSize(int w,int h);
		void setBoomRate(fl bRate1,fl bRate2);
		Sweeper():Grid() {}
		//helpful functions
		int countOf(int x,int y,int obj=objBoom);
		int mirCountOf(int x,int y,int obj=objBoom);
		bool inRound(int p1,int p2);
		void showSweeper(bool hideBooms=false);
		void showSweeperSolver(bool hideBooms=false);
		void print(bool hideBooms=true);
		//parameters
		fl boomRate1,boomRate2;
		int boomCount,beCount,tipnumCount;
		int gridCount;//equal to rndPos.size()
		vector<int> rndPos;
		vector<vector<int> > roundNumbers,roundBes;
		vector<int> boomcnt,lastcnt;
		vector<vector<int> > nearNumbers;
		//
		struct _np {
			int p1,p2,pre,nex;
		};
		int npRear;
		vector<_np> numberPairs;
		vector<int> npDir,npRev;
		int npMixed(int p1,int p2);
		void npDelete(int idx);
		void npRestore(int idx);
		void npDelete(int p1,int p2);
		void npRestore(int p1,int p2);
		//
		void getRndPos();
		void setBooms();
		void verifyBooms();
		void newRawSweeper();
		//
		int deletePrecheck(int pos);
		void deleteTipnum(int idx,int &tmp);
		void restoreTipnum(int idx,int &tmp);
		void decreaseTipnums();
		//
		void getMirGrid();
		void getSurroundedInformation();
		void getCountInformation();
		void getNearNumbers();
		void getNumberPairs();
		void newSweeper();
		
		//solve
		vector<int> _mirGrid;
		int &mirGrid(int p);
		int &mirGrid(int x,int y);
		//
		bool tryOpen(int pos);
		bool tryFlag(int pos);
		int statErrorCount,statBoomCount;
		void _open(int pos);
		void _flag(int pos);
		void undoOpen(int pos);
		void undoFlag(int pos);
		void remask(int pos);
		//
		int tpcnt;
		vector<int> tipnumPos;
		void getTipnumInformation();
		//
		int tryDirectNumber(int p1,vector<int> &changes);
		int tryTrickPair(int p1,int p2,vector<int> &changes);
		vector<vector<int> > dfsTmp,dfsTmp2;
		bool presolve(int pos,vector<int> &tmp,vector<int> &npChanges);
		//
		bool succ;
		vector<int> maskSequence;
		void dfs(int deep,bool changed);
		void getMaskSequence();
		bool solve();
	
	};//end class Sweeper
	//vector<int> _mirGrid;
	int& Sweeper::mirGrid(int p) {
		return _mirGrid[p];
	}
	int& Sweeper::mirGrid(int x,int y) {
		return _mirGrid[POS(x,y)];
	}
	//try mask->empty
	bool Sweeper::tryOpen(int pos) {
		for(int p:roundNumbers[pos])
			if(lastcnt[p]<=mirGrid(p))
				return false;
		return _open(pos),true;
	}
	bool Sweeper::tryFlag(int pos) {
		for(int p:roundNumbers[pos])
			if(boomcnt[p]>=mirGrid(p))
				return false;
		return _flag(pos),true;
	}
	//int statErrorCount,statBoomCount;
	void Sweeper::_open(int pos) {
		Assert(mirGrid(pos)==objMask);
		mirGrid(pos)=objEmpty;
		for(int p:roundNumbers[pos])
			lastcnt[p]--;
	}
	void Sweeper::_flag(int pos) {
		Assert(mirGrid(pos)==objMask);
		mirGrid(pos)=objBoom;
		for(int p:roundNumbers[pos])
			boomcnt[p]++;
		grid(pos)==objBoom?statBoomCount++:statErrorCount++;
	}
	void Sweeper::undoOpen(int pos) {
		Assert(mirGrid(pos)==objEmpty);
		mirGrid(pos)=objMask;
		for(int p:roundNumbers[pos])
			lastcnt[p]++;
	}
	void Sweeper::undoFlag(int pos) {
		Assert(mirGrid(pos)==objBoom);
		mirGrid(pos)=objMask;
		for(int p:roundNumbers[pos])
			boomcnt[p]--;
		grid(pos)==objBoom?statBoomCount--:statErrorCount--;
	}
	void Sweeper::remask(int pos) {
		mirGrid(pos)==objEmpty?undoOpen(pos):undoFlag(pos);
	}
	//vector<int> tipnumPos;
	//int tpcnt;
	void Sweeper::getTipnumInformation() {
		tipnumPos.resize(tipnumCount);
		for(int i=beCount,j=0; i<gridCount; ++i,++j)
			tipnumPos[j]=rndPos[i];
		tpcnt=tipnumPos.size();
	}
	//0: keep   1: useless   2: succ   3: err
	int Sweeper::tryDirectNumber(int p1,vector<int> &changes) {
		if(boomcnt[p1]>mirGrid(p1) || mirGrid(p1)>lastcnt[p1])
			return 3;
		if(lastcnt[p1]==boomcnt[p1])return 1;
		//p1旁边未打开的位置数量 刚好等于数字 则将这些位置标记为雷
		else if(lastcnt[p1]==mirGrid(p1)) {
			for(int p2:roundBes[p1])
			if(mirGrid(p2)==objMask) {
				if(tryFlag(p2))changes.push_back(p2);
				else return 3;
			}
			return 2;
		}
		//p1旁边已标记为雷的数量 刚好等于数字 则将这些位置标记为空
		else if(boomcnt[p1]==mirGrid(p1)) {
			for(int p2:roundBes[p1])
			if(mirGrid(p2)==objMask) {
				if(tryOpen(p2))changes.push_back(p2);
				else return 3;
			}
			return 2;
		}
		else return 0;
	}
	int Sweeper::tryTrickPair(int p1,int p2,vector<int> &changes) {
		if(lastcnt[p1]==boomcnt[p1] || lastcnt[p2]==boomcnt[p2])
			return 1;
		//
		int n1=mirGrid(p1)-boomcnt[p1];
		int n2=mirGrid(p2)-boomcnt[p2];
		static vector<int> s1,s2,sx;
		s1.resize(0),s2.resize(0),sx.resize(0);
		//s1为独属于p1周围的mask, s2为独属于p2周围的mask, sx为交集部分
		for(int p:roundBes[p1])
		if(mirGrid(p)==objMask) {
			if(inRound(p,p2))sx.push_back(p);
			else s1.push_back(p);
		}
		if(sx.size()<=1)return 1;
		for(int p:roundBes[p2])
		if(mirGrid(p)==objMask && !inRound(p,p1))
			s2.push_back(p);
		if(s1.empty() && s2.empty())return 1;
		//
		if(n1>n2 || (n1==n2 && s1.size()<s2.size())) {
			swap(n1,n2);
			s1.swap(s2);
		}
		if((int)s2.size()==n2-n1) {
			for(int p:s1) {
				if(tryOpen(p))changes.push_back(p);
				else return 3;
			}
			for(int p:s2) {
				if(tryFlag(p))changes.push_back(p);
				else return 3;
			}
			return 2;
		}
		return 0;
	}
	bool Sweeper::presolve(int pos,vector<int> &changes,vector<int> &npChanges) {
		bool updated=false;
		if(pos==-1)updated=true;
		else {
			for(int p1:roundNumbers[pos]) {
				int stat=tryDirectNumber(p1,changes);
				if(stat==3)return false;
				if(stat==2)updated=true;
			}
			for(int p1:roundNumbers[pos])
			if(mirGrid(p1) && lastcnt[p1]!=boomcnt[p1]) {
				for(int p2:nearNumbers[p1])
				if(lastcnt[p2]!=boomcnt[p2]) {
					int stat=tryTrickPair(p1,p2,changes);
					if(stat==3)return false;
					if(stat==2)updated=true;
				}
			}
		}
		//
		while(updated) {
			while(updated) {
				updated=false;
				for(int idx=0; idx<tpcnt; ++idx) {
					int stat=tryDirectNumber(tipnumPos[idx],changes);
					if(stat==3)return false;
					if(stat==2)updated=true;
					if(stat)swap(tipnumPos[idx--],tipnumPos[--tpcnt]);
				}
			}
			for(int idx=numberPairs[0].nex; idx!=npRear; idx=numberPairs[idx].nex) {
				int stat=tryTrickPair(numberPairs[idx].p1,numberPairs[idx].p2,changes);
				if(stat==3)return false;
				if(stat==2)updated=true;
				if(stat) {
					npDelete(idx);
					npChanges.push_back(idx);
				}
			}
		}
		return true;
	}
	//bool succ;
	//vector<int> maskSequence;
	//vector<vector<int> > dfsTmp,dfsTmp2;
	void Sweeper::dfs(int deep,bool changed) {
		if(deep==beCount) {//maskSequence.size()
			succ=(statErrorCount==0 && statBoomCount==boomCount);
			if(generateBoomsGetInfo) {
				generateBoomsGetInfo=false;
				succ=false;
				//
				for(int j=0; j<boardHeight; ++j)
				for(int i=0; i<boardWidth; ++i) {
					int obj=mirGrid(i+1,j+1);
					if(obj==generateBoomsLogic::objBoom)
						boomMap(i,j)=tile_boom;
					else if(obj==generateBoomsLogic::objEmpty)
						boomMap(i,j)=tile_0;
				}
			}
			return;
		}
		int pos=maskSequence[deep];
		//
		int tpcnt_rec=tpcnt;
		dfsTmp[deep].resize(0);
		dfsTmp2[deep].resize(0);
		if(changed && !presolve(deep?maskSequence[deep-1]:-1,dfsTmp[deep],dfsTmp2[deep]))
			goto END;
		if(mirGrid(pos)!=objMask) {
			dfs(deep+1,false);
			goto END;
		}
		//
		if(tryFlag(pos)) {
			dfs(deep+1,true);
			undoFlag(pos);
			if(!succ)goto END;
		}
		if(tryOpen(pos)) {
			dfs(deep+1,true);
			undoOpen(pos);
			if(!succ)goto END;
		}
		END:tpcnt=tpcnt_rec;
		for(int p:dfsTmp[deep])remask(p);
		for(int i=(int)dfsTmp2[deep].size()-1; i>=0; --i)
			npRestore(dfsTmp2[deep][i]);
	}
	//
	void Sweeper::getMaskSequence() {
		//get mask sequence
		maskSequence.reserve(gridCount);
		maskSequence.resize(beCount);
		for(int j=1,cnt=0; j<=height; ++j)
		for(int i=1; i<=width; ++i)
		if(mirGrid(i,j)==objMask)
			maskSequence[cnt++]=POS(i,j);
	//	for(int i=0; i<beCount; ++i)
	//		maskSequence[i]=rndPos[i];
	//	shuffle(&maskSequence[0],beCount);
		//
		dfsTmp.reserve(gridCount);
		dfsTmp2.reserve(gridCount);
		dfsTmp.resize(maskSequence.size());
		dfsTmp2.resize(maskSequence.size());
	}
	//result: if unique solve
	bool Sweeper::solve() {
		//inits
		statErrorCount=statBoomCount=0;
		succ=true;
		getMaskSequence();
		getTipnumInformation();
		//run
		dfs(0,true);
		return succ;
	}
	//
	const char charObjects[32]="012345678- *//";
	int& Sweeper::grid(int p) {
		return _grid[p];
	}
	int& Sweeper::grid(int x,int y) {
		return _grid[POS(x,y)];
	}
	bool Sweeper::inGrid(int x,int y) {
		return objOn(x,y)!=objVoid;
	}
	int Sweeper::size() const {
		return _grid.size();
	}
	void Sweeper::setSize(int w,int h) {
		if(!(0<w && w<1e4 && 0<h && h<1e4 && w*h<1e6)) {
			cerr<<"\nError: invalid sweeper size("<<w<<","<<h<<").\n";
			exit(999);
		}
		width=w,height=h,borderW=1;
	}
	void Sweeper::setBoomRate(fl bRate1,fl bRate2) {
		if(!(0<=bRate1 && bRate1<1 && 
			0<=bRate2 && bRate2<1 && bRate1<=bRate2))
		{
			cerr<<"\nError: invalid boom rate("<<bRate1<<","<<bRate2<<").\n";
			exit(999);
		}
		boomRate1=bRate1,boomRate2=bRate2;
	}
	//count objs surround of (x,y)
	int Sweeper::countOf(int x,int y,int obj) {
		int cnt=0;
		for(int dir=0; dir<8; ++dir) {
			int tx=x+dx8[dir],ty=y+dy8[dir];
			if(inGrid(tx,ty) && (obj==objVarious || grid(tx,ty)==obj))
				cnt++;
		}
		return cnt;
	}
	int Sweeper::mirCountOf(int x,int y,int obj) {
		int cnt=0;
		for(int dir=0; dir<8; ++dir) {
			int tx=x+dx8[dir],ty=y+dy8[dir];
			if(inGrid(tx,ty) && (obj==objVarious || mirGrid(tx,ty)==obj))
				cnt++;
		}
		return cnt;
	}
	bool Sweeper::inRound(int p1,int p2) {
		int x1,y1,x2,y2;
		XY(p1,x1,y1),XY(p2,x2,y2);
		return abs(x1-x2)<2 && abs(y1-y2)<2;
	}
	void Sweeper::showSweeper(bool hideBooms) {
		cout<<endl<<"   ";
		for(int i=1; i<=width; ++i)cout<<i%10<<" ";
		cout<<endl<<endl;
		for(int j=1; j<=height; ++j) {
			cout<<j%10<<"  ";
			for(int i=1; i<=width; ++i) {
				int obj=grid(i,j);
				if(hideBooms && (obj==objBoom || obj==objEmpty))cout<<"H ";
				else cout<<charObjects[obj]<<" ";
			}
			cout<<endl;
		}
	}
	void Sweeper::showSweeperSolver(bool hideBooms) {
		cout<<endl;
		for(int j=1; j<=height; ++j) {
			for(int i=1; i<=width; ++i) {
				int obj=mirGrid(i,j);
				if(hideBooms && (obj==objBoom || obj==objEmpty))cout<<"H ";
				else cout<<charObjects[obj]<<" ";
			}
			cout<<endl;
		}
	}
	void Sweeper::print(bool hideBooms) {
		return;
	}
	//
	//fl boomRate1,boomRate2;
	//int boomCount,beCount,tipnumCount;
	//int gridCount; //equal to rndPos.size()
	//vector<int> rndPos;
	void Sweeper::getRndPos() {
		//get rndPos: random_valid_pos_seq
		// & gridCount: valid_grid_cnt(not void)
		rndPos.reserve(height*width);
		rndPos.resize(0);
		for(int j=1; j<=height; ++j)
		for(int i=1; i<=width; ++i)
		if(inGrid(i,j))
			rndPos.push_back(POS(i,j));
		gridCount=rndPos.size();
		shuffle(rndPos.begin(),rndPos.end(),rng);
	}
	void Sweeper::setBooms() {
		//决定雷数(boomCount)
		//同时初始化beCount(boom_or_empty_cnt) & tipnumCount.
		int bcnt1=round(boomRate1*gridCount),bcnt2=round(boomRate2*gridCount);
		boomCount=rng()%(bcnt2-bcnt1+1)+bcnt1;
		beCount=boomCount,tipnumCount=gridCount-beCount;
		//将前boomCount个位置指定为雷
		for(int i=0; i<boomCount; ++i)
			grid(rndPos[i])=objBoom;
	}
	void Sweeper::verifyBooms() {
		//此时grid中全为void/booms/empty
		//防止雷被雷包围: 其中的雷将不被任何数字记录
		for(int i=0,j=boomCount,x,y,tx,ty; i<boomCount; ++i) {
			XY(rndPos[i],x,y);
			if(countOf(x,y,objEmpty)==0) {
				grid(rndPos[i])=objEmpty;
				while(j<gridCount) {
					XY(rndPos[j],tx,ty);
					if(countOf(tx,ty,objEmpty)>0) {//
						for(int dir=0; dir<8; ++dir) {
							int tx2=tx+dx8[dir],ty2=ty+dy8[dir];
							if(grid(tx2,ty2)==objBoom)
							if(countOf(tx2,ty2,objEmpty)==1)
								goto NextEmpty;
						}
						grid(rndPos[j])=objBoom;
						swap(rndPos[i],rndPos[j]);
						goto NextBoom;
					}
					NextEmpty:j++;
				}
				cerr<<"\nGeneration failed due to eternal multi-solve: "
									<<boomCount<<"/"<<gridCount<<".\n";
				exit(999);
			}
			NextBoom:;
		}
	}
	//获得grid, rndPos, 雷和全部提示数
	void Sweeper::newRawSweeper() {
		if(initGrid(width,height,1,objEmpty,objVoid)<0) {
			return;
		}
		//get canvas
		if(0) {
			fillCenter(objVoid);
			int avgside=(height+width)/2;
			int ccnt=rng()%(avgside/6+1)+avgside/2.4;
			int cCenter[ccnt],cRadius[ccnt];
			for(int i=0; i<ccnt; ++i) {
				int x=rng()%width+1,y=rng()%height+1;
				cCenter[i]=POS(x,y);
				cRadius[i]=rng()%(avgside/6+1)+avgside/12+1;
			}
			vector<double> noise(size());
			for(int j=1; j<=height; ++j)
			for(int i=1; i<=width; ++i) {
				for(int k=0,x,y; k<ccnt; ++k) {
					XY(cCenter[k],x,y);
					double sqr=(x-i)*(x-i)+(y-j)*(y-j);
					if(cRadius[k]>sqrt(sqr))
						noise[POS(i,j)]+=sqrt(cRadius[k]*cRadius[k]+sqr);
				}
				if(noise[POS(i,j)]>avgside*avgside/110.)
					grid(i,j)=objEmpty;
			}
		}
		//than, empty filled by booms & numbers
		getRndPos();
		setBooms();
		verifyBooms();
		//设置提示数
		for(int i=boomCount,x,y; i<gridCount; ++i) {
			Assert(grid(rndPos[i])==objEmpty);
			XY(rndPos[i],x,y);
			grid(rndPos[i])=countOf(x,y);
		}
	}
	//vector<vector<int> > roundNumbers,roundBes;
	void Sweeper::getSurroundedInformation() {
		//get surrounded boom/empty & numbers information
		roundNumbers.resize(size());
		roundBes.resize(size());
		for(int i=0; i<gridCount; ++i) {
			roundNumbers[rndPos[i]].reserve(8);
			roundNumbers[rndPos[i]].resize(0);
		}
		for(int i=boomCount; i<gridCount; ++i) {
			roundBes[rndPos[i]].reserve(8);
			roundBes[rndPos[i]].resize(0);
		}
		//all grids <-> all grids
		for(int pos:rndPos) {
			int x,y;
			XY(pos,x,y);
			for(int dir=0; dir<8; ++dir) {
				int tx=x+dx8[dir],ty=y+dy8[dir];
				int p=POS(tx,ty),obj=grid(p);
				if(obj==objBoom)roundBes[pos].push_back(p);
				else if(isObjNumber(obj))roundNumbers[pos].push_back(p);
			}
		}
	}
	//vector<int> boomcnt,lastcnt;
	void Sweeper::getCountInformation() {
		//此时mirGrid中仅有void/mask/number
		boomcnt.resize(size());
		lastcnt.resize(size());
		for(int i=0; i<gridCount; ++i) {//i could from boomCount
			int p=rndPos[i];
			lastcnt[p]=roundBes[p].size();
		}
	}
	//vector<vector<int> > nearNumbers;
	void Sweeper::getNearNumbers() {
		//p1num!=0 && p2num!=0 && not corner
		nearNumbers.resize(size());
		for(int p:rndPos) {
			nearNumbers[p].reserve(20);
			nearNumbers[p].resize(0);
		}
		for(int i=0,x,y; i<gridCount; ++i) {
			int p1=rndPos[i],obj=grid(p1);
			if(!(1<=obj && obj<=8))continue;
			XY(p1,x,y);
			for(int dir=1; dir<24; ++dir)
			if((dir&7)!=4) {
				int tx=x+dir%5-2,ty=y+dir/5-2;
				if(::inBoard(tx,ty,1,1,width+1,height+1)) {
					int p2=POS(tx,ty),obj=grid(p2);
					if(1<=obj && obj<=8)
						nearNumbers[p1].push_back(p2);
				}
			}
		}
	}
	//struct _np;
	//int npRear;
	//vector<_np> numberPairs;
	//vector<int> npDir,npRev;
	int Sweeper::npMixed(int p1,int p2) {
		if(p1>p2)swap(p1,p2);
		return p1<<4|npDir[p2-p1];
	}
	void Sweeper::npDelete(int idx) {
		_np &np=numberPairs[idx];
		if(numberPairs[np.pre].nex==idx) {
			//	Assert(numberPairs[np.nex].pre==idx);//
			numberPairs[np.pre].nex=np.nex;
			numberPairs[np.nex].pre=np.pre;
		}
	}
	void Sweeper::npRestore(int idx) {
		_np &np=numberPairs[idx];
		if(numberPairs[np.pre].nex!=idx) {
			//	Assert(numberPairs[np.nex].pre!=idx);//
			numberPairs[np.pre].nex=idx;
			numberPairs[np.nex].pre=idx;
		}
	}
	void Sweeper::npDelete(int p1,int p2) {
		npDelete(npRev[npMixed(p1,p2)]);
	}
	void Sweeper::npRestore(int p1,int p2) {
		npRestore(npRev[npMixed(p1,p2)]);
	}
	void Sweeper::getNumberPairs() {
		//init npDir
		for(int dir=1; dir<12; ++dir) {
			int ty=dir/5,tx=dir%5;
			int diff=POS(2,2)-POS(tx,ty);
			if((int)npDir.size()<diff)npDir.resize(diff+1);
			npDir[diff]=dir;
		}
		//rev: pair -> seq idx
		npRev.resize(POS(width,height)<<4);
		//
		numberPairs.reserve(tipnumCount*10+2);
		numberPairs.resize(0);
		//npHead: 0
		numberPairs.push_back({-1,-1,-1,1});
		npRear=1;
		for(int i=boomCount; i<gridCount; ++i) {
			int p1=rndPos[i];
			if(grid(p1)!=0)
			for(int p2:nearNumbers[p1])
			if(p1>p2) {
				npRev[npMixed(p2,p1)]=npRear++;
				numberPairs.push_back({p2,p1,npRear-2,npRear});
			}
		}
		numberPairs.push_back({-1,-1,npRear-1,-1});
	}
	void Sweeper::getMirGrid() {
		//grid -> mirGrid : hide B/E
		_mirGrid.resize(size());
		for(int j=0; j<=height+1; ++j)
		for(int i=0; i<=width+1; ++i) {
			int t=grid(i,j);
			if(t==objBoom || t==objEmpty)
				t=objMask;
			mirGrid(i,j)=t;
		}
	}
	//0: unknow   1: yes   2: no
	int Sweeper::deletePrecheck(int pos) {
		//旁边全是数字 直接删掉不会产生多解x
		//if(!roundNumbers[pos].empty() && roundBes[pos].empty())return 1;
		//旁边没有数字 则不能删
		if(roundNumbers[pos].empty())return 2;
		//旁边的某个雷或空仅被当前数字可见 不可删
		for(int p:roundBes[pos])
		if(roundNumbers[p].size()==1)
			return 2;
		//若某个雷和删数之后的空交换不发生影响 即产生多解
		int x,y;
		XY(pos,x,y);
		for(int dir=0; dir<25; ++dir)
		if(dir!=12) {
			int tx=x+dir%5-2,ty=y+dir/5-2,p=POS(tx,ty);
			if(::inBoard(tx,ty,1,1,width+1,height+1) && grid(p)==objBoom) {
				for(int p2:roundNumbers[p])
				if(!inRound(p2,pos))
					goto nextboom;
				if(roundNumbers[pos].size()+(abs(x-tx)<2 && abs(y-ty)<2)
												==roundNumbers[p].size())
					return 2;
			}
			nextboom:;
		}
		return 0;
	}
	void Sweeper::deleteTipnum(int idx,int &tmp) {
		//删掉一个提示数: number -> empty
		int pos=rndPos[idx],x,y;
		XY(pos,x,y);
		for(int dir=0; dir<8; ++dir) {
			int tx=x+dx8[dir],ty=y+dy8[dir];
			if(inGrid(tx,ty)) {
				int p=POS(tx,ty);
				//删掉旁边格子对这个格子的number记录
				for(auto it=roundNumbers[p].begin(); ; ++it)
				if(*it==pos) {//it!=roundNumbers[p].end()
					roundNumbers[p].erase(it);
					break;
				}
				//加上旁边格子对这个格子的be记录
				roundBes[p].push_back(pos);
				//旁边格子的lastcnt+1
				lastcnt[p]++;
			}
		}
		//删除附近数字对这个数字的nearNumbers记录
		for(int p:nearNumbers[pos]) {
			for(auto it=nearNumbers[p].begin(); ; ++it) {
				Assert(it!=nearNumbers[p].end());
				if(*it==pos) {//
					nearNumbers[p].erase(it);
					break;
				}
			}
		}
		//del np rec
		for(int p:nearNumbers[pos])npDelete(p,pos);
		//
		tmp=grid(pos);
		grid(pos)=objEmpty;
		mirGrid(pos)=objMask;
		swap(rndPos[idx],rndPos[beCount++]);
		tipnumCount--;
	}
	void Sweeper::restoreTipnum(int idx,int &tmp) {
		//恢复提示数
		swap(rndPos[idx],rndPos[--beCount]);
		int pos=rndPos[idx],x,y;
		grid(pos)=mirGrid(pos)=tmp;
		XY(pos,x,y);
		for(int dir=0; dir<8; ++dir) {
			int tx=x+dx8[dir],ty=y+dy8[dir];
			if(inGrid(tx,ty)) {
				int p=POS(tx,ty);
				//恢复旁边格子对这个格子的number记录
				roundNumbers[p].push_back(pos);
				//删除旁边格子对这个格子的be记录
				for(auto it=roundBes[p].begin(); ; ++it)
				if(*it==pos) {//it!=roundBes[p].end()
					roundBes[p].erase(it);
					break;
				}
				//旁边格子lastcnt-1
				lastcnt[p]--;
			}
		}
		//恢复附近数字对这个数字的nearNumbers记录
		for(int p:nearNumbers[pos])
			nearNumbers[p].push_back(pos);
		//restore np rec
		for(int idx=(int)nearNumbers[pos].size()-1; idx>=0; --idx)
			npRestore(nearNumbers[pos][idx],pos);
		tipnumCount++;
	}
	//去掉可去的提示数: 替换为empty
	void Sweeper::decreaseTipnums() {
		// Assert(solve());//
		cout<<"\nDecreasing tip number count: "<<tipnumCount;
		//try delete tip numbers
		for(int i=boomCount,tmp; i<gridCount; ++i) {
			int stat=deletePrecheck(rndPos[i]);
			if(stat<2)deleteTipnum(i,tmp);
			if(stat==0) {
				if(solve())stat=1;
				else {
					restoreTipnum(i,tmp);
					stat=2;
				}
			}
			if(stat==1) {
				cout<<" "<<tipnumCount;
			}
			else {
				cout<<".";
			}
		}
		cout<<"\nDone.\n";
		//	Assert(solve());//
	}
	//this func aim to get a minesweeper: grid fill of void/booms/number/empty
	void Sweeper::newSweeper() {
		clock_t t0=clock();//此时grid中为void/empty
		newRawSweeper();//此时grid中没有empty, 全部被void/booms/number填满
		getSurroundedInformation();
		getCountInformation();
		getNearNumbers();
		getNumberPairs();
		getMirGrid();//grid没有变化
		// showSweeper();//
		decreaseTipnums();//grid中部分数字变成了empty, 生成完成
		//

		cout<<fixed<<setprecision(2);
		cout<<"\nBoom count: "<<boomCount<<"/"<<gridCount<<endl;
		cout<<"Tipnum count: "<<tipnumCount<<"/"<<gridCount<<endl;
		cout<<"\nGeneration time used "<<(clock()-t0)/1000.<<"s."<<endl;
	}
	//
	void Main() {
		Sweeper sweeper;
		sweeper.setSize(boardWidth,boardHeight);
		fl rate=(fl)maxBooms/boardSize;
		sweeper.setBoomRate(rate,rate);
		sweeper.newSweeper();
		//
		for(int j=0; j<boardHeight; ++j)
		for(int i=0; i<boardWidth; ++i) {
			int obj=sweeper.grid(i+1,j+1);
			if(obj<=objNumberMax)boomMap(i,j)=obj;
			else if(obj==objBoom)boomMap(i,j)=tile_boom;
			else if(obj==objEmpty)boomMap(i,j)=tile_empty;
			else Assert(false);
		}
		for(int j=0; j<boardHeight; ++j)
		for(int i=0; i<boardWidth; ++i) {
			int tile=boomMap(i,j);
			if(tile==tile_boom || tile==tile_empty)
				maskedMap(i,j)=tile_mask;
			else maskedMap(i,j)=tile;
		}
	}
}
namespace generateBoomsMagic {
	bool Main(int x=-1,int y=-1,bool v=0) {
		if(x==-1) {
			return false;
		}
		//tmpMap: flag,mask -> mask
		Board tmpMap=maskedMap;
		Assert(sweeperMode==0 && tmpMap(x,y)==tile_mask && v==false);
		for(int j=0; j<boardHeight; ++j)
		for(int i=0; i<boardWidth; ++i)
		if(tmpMap(i,j)==tile_flag)
			tmpMap(i,j)=tile_mask;
		//tmpMap -> sweeper
		generateBoomsLogic::Sweeper sweeper;
		sweeper.setSize(boardWidth,boardHeight);
		Assert(sweeper.initGrid(sweeper.w(),sweeper.h(),1
				,generateBoomsLogic::objEmpty,generateBoomsLogic::objVoid)>=0);
		vector<PII> backForBoomMasks;
		for(int j=0; j<boardHeight; ++j)
		for(int i=0; i<boardWidth; ++i) {
			int tile=tmpMap(i,j);
			if(tile==tile_mask) {
				bool hasNumberNeighbor=false;
				for(PII &neighbor:surroundCoords(i,j))
				if(tmpMap(neighbor)!=tile_mask)
					hasNumberNeighbor=true;
				if(hasNumberNeighbor)
					sweeper.grid(i+1,j+1)=generateBoomsLogic::objBoom;
				else if(!(i==x && j==y)) {
					if(boomMap(i,j)!=tile_boom)
						backForBoomMasks.push_back(PII(i+1,j+1));
					sweeper.grid(i+1,j+1)=generateBoomsLogic::objVoid;
				}
			}
			else {
				Assert(tile_0<=tile && tile<=tile_8);
				sweeper.grid(i+1,j+1)=tile-tile_0;
			}
		}
		sweeper.grid(x+1,y+1)=generateBoomsLogic::objVoid;
		if(backForBoomMasks.size()) {
			PII pos=backForBoomMasks[rng()%backForBoomMasks.size()];
			sweeper.grid(pos.first,pos.second)=generateBoomsLogic::objBoom;
		}
		cout<<"de1"<<endl;
		sweeper.showSweeper();
		//find solve
		int tipnumCount=0;
		for(int j=1,J=sweeper.h(); j<=J; ++j)
		for(int i=1,I=sweeper.w(); i<=I; ++i)
		if(sweeper.grid(i,j)<=generateBoomsLogic::objNumberMax)
			tipnumCount++;
		sweeper.rndPos.resize(0);
		for(int j=1,J=sweeper.h(); j<=J; ++j)
		for(int i=1,I=sweeper.w(); i<=I; ++i)
		if(sweeper.inGrid(i,j))
			sweeper.rndPos.push_back(sweeper.POS(i,j));
		sweeper.gridCount=sweeper.rndPos.size();
		shuffle(sweeper.rndPos.begin(),sweeper.rndPos.end(),rng);
		for(int i=0,I=sweeper.rndPos.size(); i<I; ++i) {
			int p=sweeper.rndPos[i];
			if(sweeper.grid(p)<=generateBoomsLogic::objNumberMax)
				swap(sweeper.rndPos[i--],sweeper.rndPos[--I]);
		}
		sweeper.beCount=sweeper.gridCount-tipnumCount;
		sweeper.tipnumCount=sweeper.gridCount-sweeper.beCount;
		sweeper.getSurroundedInformation();
		sweeper.getCountInformation();
		sweeper.getNearNumbers();
		sweeper.getNumberPairs();
		sweeper.getMirGrid();
		generateBoomsGetInfo=true;
		sweeper.solve();
		if(generateBoomsGetInfo) {
			generateBoomsGetInfo=false;
			return false;
		}
		else {
			boomMap(x,y)=tile_0;
			_getNumbers();
			cout<<"de2"<<endl;
			boomMap.show();
			return true;
		}
	}
}
void initGame_core(int mode,int diffculty,bool geneBooms) {
	sweeperMode=mode,gameDiffculty=diffculty;
	if(gameDiffculty==gameDiffcultyEasy)
		boardWidth=9,boardHeight=9,maxBooms=10;
	else if(gameDiffculty==gameDiffcultyNormal)
		boardWidth=16,boardHeight=16,maxBooms=40;
	else if(gameDiffculty==gameDiffcultyHard)
		boardWidth=30,boardHeight=16,maxBooms=99;
	else if(gameDiffculty==gameDiffcultyCustom)
		boardWidth=customBoardW,boardHeight=customBoardH;
	else Assert(false);
	boardSize=boardWidth*boardHeight;
	if(gameDiffculty==gameDiffcultyCustom) {
		if(sweeperMode==sweeperModeClassic)
			maxBooms=round(customBoomRatio*boardSize);
		else {
			int L=round((customBoomRatio-0.02)*boardSize);
			int R=round((customBoomRatio+0.02)*boardSize);
			L=max(1,L),R=min(boardSize-1,R);
			if(L>R)maxBooms=0;
			else maxBooms=L+rng()%(R-L+1);
		}
	}
	//
	boomMap.resize(boardSize);
	maskedMap.resize(boardSize);
	if(geneBooms) {
		if(sweeperMode==1) generateBoomsLogic::Main();
		else generateBooms();
	}
}

}	//end namespace core
using namespace core;



namespace graphics {

int cellW,spacing,cellCW;
int fontH,emojiW,emojiW2;
int panelX,panelY,emojiX,emojiY;
int windowWidth,windowHeight;
Gdiplus::Bitmap *imgMaskOrg,*imgEmptyOrg,*imgOrangeOrg,*imgFlagOrg;
Gdiplus::Bitmap *imgMask,*imgEmpty,*imgOrange,*imgFlag;
Gdiplus::Bitmap *imgEmojisOrg[_emojiCount],*imgEmojis[_emojiCount];
#define BGR(c) ((((c)&0xFF)<<16)|((c)&0xFF00FF00)|(((c)&0xFF0000)>>16))
COLORREF textNormalColor=RGB(252,240,240);
COLORREF textErrorColor=RGB(40,40,40);
COLORREF orangeColor=RGB(234,126,50);
COLORREF colorMix(COLORREF c1,COLORREF c2,double _ratio) {
	int ratio=round(255.*min(1.,max(0.,_ratio)));
	int r1=GetRValue(c1),g1=GetGValue(c1),b1=GetBValue(c1);
	int r2=GetRValue(c2),g2=GetGValue(c2),b2=GetBValue(c2);
	int r=(r1*(255-ratio)+r2*ratio+127)/255;
	int g=(g1*(255-ratio)+g2*ratio+127)/255;
	int b=(b1*(255-ratio)+b2*ratio+127)/255;
	Assert(0<=r && 0<=g && 0<=b);
	Assert(r<=255 && g<=255 && b<=255);
	return RGB(r,g,b);
}
int center(int scrW,int objW) {
	return (scrW-objW)/2;
}
int getScreenW() {
	return GetSystemMetrics(SM_CXSCREEN);
}
int getScreenH() {
	return GetSystemMetrics(SM_CYSCREEN);
}
//
bool convertCoord(int &x,int &y) {
	x-=panelX,y-=panelY;
	if(x<0 || y<0)return false;
	x/=cellW,y/=cellW;
	return inBoard(x,y);
}
int calcWindowWidth(int _cellW) {
	return (boardWidth+1)*_cellW;
}
int calcWindowHeight(int _cellW) {
	return ceil((boardHeight+2.5)*_cellW+0.1);
}
int getDefCellW() {
	int scrW=GetSystemMetrics(SM_CXSCREEN);
	int scrH=GetSystemMetrics(SM_CYSCREEN);
	if(DEBUG)cout<<"screen w,h: "<<scrW<<" "<<scrH<<endl;
	return round(min(scrW,scrH)*0.05);
}
void initLayout(int _cellW,int w=0,int h=0) {
	cellW=_cellW;
	spacing=max(1,(int)round(cellW*0.05));
	cellCW=cellW-spacing*2;
	emojiW=round(cellW*1.25),emojiW2=round(cellW*1.5);
	fontH=max(6,(int)floor(cellW*0.9));
	if(w<=0) {
		windowWidth=calcWindowWidth(cellW);
		windowHeight=calcWindowHeight(cellW);
	}
	else windowWidth=w,windowHeight=h;
	panelX=center(windowWidth,boardWidth*cellW);
	panelY=center(windowHeight,boardHeight*cellW+emojiW2)+emojiW2;
	emojiX=center(windowWidth,emojiW);
	emojiY=panelY-emojiW2+center(emojiW2,emojiW);
}
void onSize(BufferContext &ctx,int w,int h) {
	int cellW1=floor(w/(boardWidth+1.)-0.1);
	int cellW2=floor(h/(boardHeight+2.5)-0.1);
	initLayout(min(cellW1,cellW2),w,h);
	//font
	LOGFONT lf={};
	lf.lfHeight=fontH;
	lf.lfWeight=FW_BOLD;
	strcpy(lf.lfFaceName,"幼圆");
	HFONT hfont=CreateFontIndirect(&lf);
	HFONT oldFont=(HFONT)SelectObject(ctx.hdcMem,hfont);
	DeleteObject(oldFont);
	SetBkMode(ctx.hdcMem,TRANSPARENT);
	SetTextColor(ctx.hdcMem,textNormalColor);
	//images
	auto resizeImage=[] (BufferContext &ctx,
		Gdiplus::Bitmap *img1,Gdiplus::Bitmap *&img2,int w,int h)
	{
		int oldw=img2->GetWidth(),oldh=img2->GetHeight();
		if(!(oldw==w && oldh==h)) {
			Gdiplus::Bitmap *scaledBitmap=
				new Gdiplus::Bitmap(w,h,PixelFormat32bppARGB);
			if(!scaledBitmap)return;
			Gdiplus::Graphics graphics(scaledBitmap);
			graphics.DrawImage(img1,0,0,w,h);
			delete img2;
			img2=scaledBitmap;
		}
	};
	resizeImage(ctx,imgOrangeOrg,imgOrange,cellW,cellW);
	resizeImage(ctx,imgFlagOrg,imgFlag,cellW,cellW);
	for(int i=0; i<_emojiCount; ++i)
		resizeImage(ctx,imgEmojisOrg[i],imgEmojis[i],emojiW,emojiW);
}
void initResource() {
	imgMask=new Gdiplus::Bitmap(L"res/leaves.png");
	imgEmpty=new Gdiplus::Bitmap(L"res/grass.png");
	imgOrange=new Gdiplus::Bitmap(L"res/orange.png");
	imgFlag=new Gdiplus::Bitmap(L"res/flag.png");
	imgMaskOrg=new Gdiplus::Bitmap(L"res/leaves.png");
	imgEmptyOrg=new Gdiplus::Bitmap(L"res/grass.png");
	imgOrangeOrg=new Gdiplus::Bitmap(L"res/orange.png");
	imgFlagOrg=new Gdiplus::Bitmap(L"res/flag.png");
	imgEmojis[emojiInit]=new Gdiplus::Bitmap(L"res/emoji_init.png");
	imgEmojis[emojiPending]=new Gdiplus::Bitmap(L"res/emoji_pending.png");
	imgEmojis[emojiFail]=new Gdiplus::Bitmap(L"res/emoji_fail.png");
	imgEmojis[emojiWin]=new Gdiplus::Bitmap(L"res/emoji_win.png");
	imgEmojisOrg[emojiInit]=new Gdiplus::Bitmap(L"res/emoji_init.png");
	imgEmojisOrg[emojiPending]=new Gdiplus::Bitmap(L"res/emoji_pending.png");
	imgEmojisOrg[emojiFail]=new Gdiplus::Bitmap(L"res/emoji_fail.png");
	imgEmojisOrg[emojiWin]=new Gdiplus::Bitmap(L"res/emoji_win.png");
}
void releaseResource() {
	delete imgMask;
	delete imgEmpty;
	delete imgOrange;
	delete imgFlag;
}
//
struct GdipBuf {
	GdipBuf(Gdiplus::Bitmap *bmp) {
		this->bmp=bmp;
		Gdiplus::Rect rect(0,0,bmp->GetWidth(),bmp->GetHeight());
		bmp->LockBits(&rect
			,Gdiplus::ImageLockModeRead|Gdiplus::ImageLockModeWrite,
			PixelFormat32bppARGB,&data);
	}
	~GdipBuf() {
		bmp->UnlockBits(&data);
	}
	DWORD &operator () (int x,int y) {
		return static_cast<DWORD*>(data.Scan0)[y*data.Stride/4+x];
	}
	Gdiplus::Bitmap *bmp;
	Gdiplus::BitmapData data;
};
void drawEmoji(BufferContext &ctx,int emojiState) {
	if(ctx.gra) {
		GdipBuf bufEmpty(imgEmpty);
		int w=imgEmpty->GetWidth(),h=imgEmpty->GetHeight();
		for(int j=emojiY,J=j+emojiW; j<J; ++j)
		for(int i=emojiX,I=i+emojiW; i<I; ++i)
			ctx(i,j)=bufEmpty(i%w,j%h);
		ctx.gra->DrawImage(imgEmojis[emojiState],emojiX,emojiY);
	}
}
void drawBackground(BufferContext &ctx) {
	if(ctx.gra) {
		// ctx.gra->DrawImage(imgEmpty,0,0,windowWidth,windowHeight);
		GdipBuf bufEmpty(imgEmpty);
		int w=imgEmpty->GetWidth(),h=imgEmpty->GetHeight();
		for(int j=0; j<windowHeight; ++j)
		for(int i=0; i<windowWidth; ++i)
			ctx(i,j)=bufEmpty(i%w,j%h);
	}
}
void drawScene(BufferContext &ctx,int emojiState
				,PII boomPos=PII(0,0),int stage=-1)
{
	drawBackground(ctx);
	drawEmoji(ctx,emojiState);
	if(ctx.gra) {
		GdipBuf bufMask(imgMask);
		int imgMaskW=imgMask->GetWidth(),imgMaskH=imgMask->GetHeight();
		for(int j=0; j<boardHeight; ++j)
		for(int i=0; i<boardWidth; ++i) {
			int tile=maskedMap(i,j);
			int x=panelX+i*cellW,y=panelY+j*cellW;
			if(tile==tile_mask || tile==tile_flag) {
				int sx=x+spacing,sy=y+spacing;
				for(int jj=0; jj<cellCW; ++jj)
				for(int ii=0; ii<cellCW; ++ii) {
					int tx=sx-panelX+ii,ty=sy-panelY+jj;
					ctx(sx+ii,sy+jj)=bufMask(tx%imgMaskW,ty%imgMaskH);
				}
				if(tile==tile_flag)
					ctx.gra->DrawImage(imgFlag,x,y);
				// else if(boomMap(i,j)==tile_boom)
				// 	ctx.gra->DrawImage(imgOrange,x,y);//
			}
			else if(tile==tile_boom) {
				if(stage==-1)ctx.gra->DrawImage(imgOrange,x,y);
				else if(boomPos==PII(i,j)) {
					if(1<=stage && stage<=4) {
						if(stage==1 || stage==3)x-=spacing;
						if(stage==2 || stage==4)x+=spacing;
						ctx.gra->DrawImage(imgOrange,x,y);
					}
				}
			}
			else if((tile_1<=tile && tile<=tile_8) || 
				(sweeperMode==1 && tile==tile_0))
			{
				char number[]="0";
				number[0]+=tile-tile_0;
				RECT rect={x,y,x+cellW,y+cellW};
				DrawText(ctx.hdcMem,number,-1,&rect
					,DT_SINGLELINE|DT_CENTER|DT_VCENTER);
			}
		}
	}
}

}	//end namespace graphics



namespace gameGraphic {

enum resourceIds {
	_IDM_START=1000,	//菜单项
	IDM_NEW_GAME,
	IDM_CUSTOM_GAME,
	IDM_EXIT,
	IDM_CLASSIC_EASY,
	IDM_CLASSIC_NORMAL,
	IDM_CLASSIC_HARD,
	IDM_LOGIC_EASY,
	IDM_LOGIC_NORMAL,
	IDM_LOGIC_HARD,
	IDM_LEFT_QUICK_OPEN,
	IDM_CLASSIC_NO_HELP,
	IDM_CLASSIC_REFRESH_ONCE,
	IDM_CLASSIC_REFRESH_EVERY,
	IDM_LOGIC_ENTER_CHECK,
	_IDC_START=2000,	//控件
	_IDA_START=3000,	//加速键
	IDA_NEW_GAME,
	IDA_CLASSIC_EASY,
	IDA_CLASSIC_NORMAL,
	IDA_CLASSIC_HARD,
	IDA_LOGIC_EASY,
	IDA_LOGIC_NORMAL,
	IDA_LOGIC_HARD,
};
enum timerIds {
	IDT_OPENCELLS=1,
	IDT_BOOMANIM=2,
};
HACCEL hAccel;
ACCEL accelTable[]={
    {FVIRTKEY,'N',IDM_NEW_GAME},
    {FVIRTKEY,'1',IDM_CLASSIC_EASY},
    {FVIRTKEY,'2',IDM_CLASSIC_NORMAL},
    {FVIRTKEY,'3',IDM_CLASSIC_HARD},
    {FVIRTKEY,'4',IDM_LOGIC_EASY},
    {FVIRTKEY,'5',IDM_LOGIC_NORMAL},
    {FVIRTKEY,'6',IDM_LOGIC_HARD},
};

class MainWindow: public BaseWindow<MainWindow> {
public:
	MainWindow(HINSTANCE hi,int w,int h,bool clientSize=0)
		:BaseWindow<MainWindow>()
	{
		if(create(hi,w,h,clientSize)!=TRUE) {
			errorPause("窗口创建失败 window create fails");
			exit(0);
		}
	}
	PCSTR className() const override {
		return "Main Window";
	}
	BOOL create(HINSTANCE hi,int w,int h,bool clientSize);//在这里面设置窗口过程为msgForward
	LRESULT WndProc(HWND,UINT,WPARAM,LPARAM) override;
	void flushBuffer(RECT *rect=0);
	int getWidth() const {return m_ctx.width;}
	int getHeight() const {return m_ctx.height;}
	uint32_t *getBuffer() const {return m_ctx.pBuffer;}
private:
	int emojiState=emojiInit;
	int classicModeHelpLevel=2;
	bool logicModeEnterCheck=false;
	bool blockAction=false;
	clock_t gameStartT,gameFinishT;
	void drawScene();
	void drawEmoji();
	void drawTipMessage(const char *s);
	bool checkWin(bool pressEnter=false);
	void initGame(int mode,int diffculty,bool initCore);
	vector<PII> openingCells;	//open cells module
	void updateOpeningCells();
	bool stopOpeningAnim();
	void openCell(int x,int y);
	int boomStage=-1;			//boom anim module
	PII clickedBoomPos;
	bool boomAnim();
	void clickAtBoom(int x,int y);
	bool lrQuickOpen=true;		//quick open module
	bool lbuttonDown=false;
	bool rbuttonDown=false;
	void quickOpen(int x,int y);
	void lclickCell(int x,int y);	//
	void rclickCell(int x,int y);
	//
	void onCreate();
	void onQuit();
	void onSizing(int modifyType,RECT *rect);
	void onSize(int flag,int width,int height);
	void onPaint(HDC hdc);
	void onTimer(int tid);
	void onCommand(int cmd);
	void onKeyDown(int vkcode);
	void onLButtonDown(int x,int y);
	void onLButtonUp(int x,int y);
	void onRButtonDown(int x,int y);
	void onRButtonUp(int x,int y);
	//
	bool m_created=false;
	RECT m_cliRect;
	BufferContext m_ctx;
};
BOOL MainWindow::create(HINSTANCE hInstance,int w,int h,bool clientSize) {
	if(m_hwnd) return TRUE+1;	//重复创建
	if(clientSize) {
		//计算实际尺寸
		RECT rcDesiredClient = { 0, 0, w, h }; // 目标客户区尺寸
		AdjustWindowRect(&rcDesiredClient, WS_OVERLAPPEDWINDOW, TRUE);
		w=rcDesiredClient.right - rcDesiredClient.left;
		h=rcDesiredClient.bottom - rcDesiredClient.top;
	}
	//注册窗口类
	WNDCLASS wc = {};
	wc.lpfnWndProc	 = msgForward;
	wc.hInstance	 = hInstance;
	wc.lpszClassName = className();
	wc.hCursor		 = LoadCursor(NULL,IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wc.hIcon		 = LoadIcon(NULL, IDI_APPLICATION);
	RegisterClass(&wc);
	//创建窗口实例
	m_hwnd=CreateWindowEx(
		0,						//Optional window styles
		className(),			//Window class
		"魔术扫雷",				//Window title
		WS_OVERLAPPEDWINDOW,	//Window style
		
		// x y w h
		CW_USEDEFAULT,CW_USEDEFAULT,w,h,
		
		NULL,			//Parent window
		NULL,			//Menu
		hInstance,		//Instance handle
		this			//Additional application data
	);
	return m_hwnd?TRUE:FALSE;
}
void MainWindow::flushBuffer(RECT *rect) {
	HDC hdc=GetDC(m_hwnd);
	//将内存DC内容拷贝到窗口
	if(rect) {
		int w=rect->right-rect->left,h=rect->bottom-rect->top;
		BitBlt(hdc,rect->left,rect->top,w,h,
				m_ctx.hdcMem,rect->left,rect->top,SRCCOPY);
	}
	else BitBlt(hdc,0,0,m_ctx.width,m_ctx.height, m_ctx.hdcMem,0,0,SRCCOPY);
	ReleaseDC(m_hwnd,hdc);
}
LRESULT MainWindow::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam) {
	switch(uMsg) {
		case WM_CREATE: {
			onCreate();
			return 0;
		}
		case WM_CLOSE: {
			DestroyWindow(hwnd);
			return 0;
		}
		case WM_DESTROY: {
			PostQuitMessage(0);
			return 0;
		}
		case WM_QUIT: {
			onQuit();
			return 0;
		}
		//
		case WM_SIZING: {
			onSizing(wParam,(RECT*)lParam);
			return 0;
		}
		case WM_SIZE: {
			onSize(wParam,LOWORD(lParam),HIWORD(lParam));
			break;
		}
		case WM_PAINT: {
			PAINTSTRUCT ps;
			HDC hdc=BeginPaint(hwnd,&ps);
			onPaint(hdc);
			EndPaint(hwnd,&ps);
			return 0;
		}
		case WM_ERASEBKGND: {
			return 0;
		}
		case WM_TIMER: {
			onTimer(wParam);
			return 0;
		}
		case WM_COMMAND: {
			onCommand(LOWORD(wParam));
			return 0;
		}
		case WM_KEYDOWN: {
			onKeyDown(wParam);
			break;
		}
		case WM_LBUTTONDOWN: {
			onLButtonDown(LOWORD(lParam),HIWORD(lParam));
			break;
		}
		case WM_RBUTTONDOWN: {
			onRButtonDown(LOWORD(lParam),HIWORD(lParam));
			break;
		}
		case WM_LBUTTONUP: {
			onLButtonUp(LOWORD(lParam),HIWORD(lParam));
			break;
		}
		case WM_RBUTTONUP: {
			onRButtonUp(LOWORD(lParam),HIWORD(lParam));
			break;
		}
	}
	return DefWindowProc(hwnd,uMsg,wParam,lParam);
}
//
void MainWindow::drawScene() {
	graphics::drawScene(m_ctx,emojiState);
	if(emojiState==emojiWin) {
		char s[20];
		sprintf(s,"用时: %.1lf s",(gameFinishT-gameStartT)/1000.);
		drawTipMessage(s);
	}
	flushBuffer();
}
void MainWindow::drawEmoji() {
	int x=graphics::emojiX,y=graphics::emojiY;
	RECT rect={x,y,x+graphics::emojiW,y+graphics::emojiW};
	graphics::drawEmoji(m_ctx,emojiState);
	flushBuffer(&rect);
}
void MainWindow::drawTipMessage(const char *s) {
	int x=graphics::emojiX+graphics::emojiW2,w=graphics::windowWidth-x;
	int y=graphics::emojiY,h=graphics::emojiW;
	RECT rect={x,y,x+w,y+h};
	DrawText(m_ctx.hdcMem,s,-1,&rect,DT_SINGLELINE|DT_LEFT|DT_VCENTER);
	flushBuffer(&rect);
}
bool MainWindow::checkWin(bool pressEnter) {
	if(logicModeEnterCheck && !pressEnter)
		return false;
	for(int j=0; j<boardHeight; ++j)
	for(int i=0; i<boardWidth; ++i) {
		int tile=maskedMap(i,j);
		bool real=(boomMap(i,j)==tile_boom);
		bool guess=(tile==tile_mask || tile==tile_flag);
		if((real && !guess) || (!real && guess)) {
			if(logicModeEnterCheck) {
				SetTextColor(m_ctx.hdcMem,graphics::textErrorColor);
				drawTipMessage("解答有误");
				SetTextColor(m_ctx.hdcMem,graphics::textNormalColor);
			}
			return false;
		}
	}
	emojiState=emojiWin;
	blockAction=true;
	stopOpeningAnim();
	KillTimer(m_hwnd,IDT_OPENCELLS);
	gameFinishT=clock();
	drawScene();
	return true;
}
void MainWindow::initGame(int mode,int diffculty,bool initCore) {
	emojiState=emojiInit;
	openingCells.resize(0);
	blockAction=false;
	boomStage=-1;
	KillTimer(m_hwnd,IDT_OPENCELLS);
	KillTimer(m_hwnd,IDT_BOOMANIM);
	if(initCore)initGame_core(mode,diffculty,true);
	lbuttonDown=rbuttonDown=false;
	if(mode==sweeperModeClassic)gameStartT=0;
	else gameStartT=clock();
}
//opening cells
void MainWindow::updateOpeningCells() {
	set<PII> newCells;
	for(const PII &pos:openingCells)
	if(boomMap(pos)==tile_0)
	for(const PII &pos2:surroundCoords(pos))
	if(maskedMap(pos2)==tile_mask) {
		openCell_core(pos2.first,pos2.second);
		newCells.insert(pos2);
	}
	openingCells=vector<PII>(newCells.begin(),newCells.end());
}
bool MainWindow::stopOpeningAnim() {
	bool res=openingCells.size();
	while(openingCells.size()) {
		for(PII pos:openingCells)openCell_core(pos.first,pos.second);
		updateOpeningCells();
	}
	return res;
}
void MainWindow::openCell(int x,int y) {
	//refresh booms
	if(sweeperMode==sweeperModeClassic && boomMap(x,y)==tile_boom)
	if((classicModeHelpLevel==1 && gameStartT==0) || classicModeHelpLevel==2)
		generateBoomsMagic::Main(x,y,0);
	int res=openCell_core(x,y);
	if(res>=1 && gameStartT==0)gameStartT=clock();
	if(res==0)return;
	else if(res==1) {
		if(sweeperMode==sweeperModeClassic) {
			Assert(openingCells.empty());
			openingCells.push_back(PII(x,y));
			SetTimer(m_hwnd,IDT_OPENCELLS,15,NULL);
		}
	}
	else if(sweeperMode==sweeperModeLogic && logicModeEnterCheck)
		maskedMap(x,y)=tile_empty;
	else clickAtBoom(x,y);
	drawScene();
}
//boom anim
bool MainWindow::boomAnim() {
	boomStage++;
	if(boomStage<=4) {
		graphics::drawScene(m_ctx,emojiState
							,clickedBoomPos,boomStage);
		flushBuffer();
		return true;
	}
	else if(boomStage<=12) {
		emojiState=emojiFail;
		double ratio=(12-boomStage)/8.;
		cout<<"ratio: "<<ratio<<endl;
		graphics::drawScene(m_ctx,emojiState
							,clickedBoomPos,boomStage);
		int w=getWidth(),h=getHeight();
		COLORREF orange=BGR(graphics::orangeColor);
		for(int j=0; j<h; ++j)
		for(int i=0; i<w; ++i) {
			auto &c=m_ctx(i,j);
			c=graphics::colorMix(c,orange,ratio);
		}
		flushBuffer();
		return true;
	}
	else {
		maskedMap(clickedBoomPos)=tile_0;
		return false;
	}
}
void MainWindow::clickAtBoom(int x,int y) {
	clickedBoomPos=PII(x,y);
	blockAction=true;
	boomStage=0;
	SetTimer(m_hwnd,IDT_BOOMANIM,40,NULL);
}
//quick open
void MainWindow::quickOpen(int x,int y) {
	int number=maskedMap(x,y)-tile_0;
	if((1<=number && number<=8) || (sweeperMode==1 && number==0)) {
		int flagCount=0;
		for(PII &pos:surroundCoords(x,y))
		if(maskedMap(pos)==tile_flag)
			flagCount++;
		if(flagCount==number) {
			if(!(sweeperMode==sweeperModeLogic && logicModeEnterCheck))
			for(PII &pos:surroundCoords(x,y))
			if(maskedMap(pos)==tile_mask && boomMap(pos)==tile_boom) {
				openCell_core(pos.first,pos.second);
				drawScene();
				clickAtBoom(pos.first,pos.second);
				return;
			}
			Assert(openingCells.empty());
			for(PII &pos:surroundCoords(x,y))
			if(maskedMap(pos)==tile_mask) {
				int res=openCell_core(pos.first,pos.second);
				openingCells.push_back(pos);
				if(res==2 && sweeperMode==sweeperModeLogic)
				if(logicModeEnterCheck)
					maskedMap(pos)=tile_empty;
			}
			if(openingCells.size()) {
				drawScene();
				if(sweeperMode==sweeperModeClassic)
					SetTimer(m_hwnd,IDT_OPENCELLS,15,NULL);
				else openingCells.resize(0);
			}
		}
	}
}
//
void MainWindow::lclickCell(int x,int y) {
	if(graphics::convertCoord(x,y)) {
		if(sweeperMode==sweeperModeClassic) {
			if(lrQuickOpen && rbuttonDown)quickOpen(x,y);
			else if(maskedMap(x,y)==tile_mask)openCell(x,y);
			else if(!lrQuickOpen)quickOpen(x,y);
			else return;
		}
		else {
			int &tile=maskedMap(x,y);
			if(logicModeEnterCheck) {
				if(tile==tile_mask || tile==tile_empty) {
					tile^=tile_mask^tile_empty;
					drawScene();
				}
				else if(tile==tile_flag)return;
				else quickOpen(x,y);
			}
			else {
				if(tile==tile_mask)openCell(x,y);
				else if(tile==tile_flag || tile==tile_empty)return;
				else quickOpen(x,y);
			}
		}
		checkWin();
	}
}
void MainWindow::rclickCell(int x,int y) {
	if(graphics::convertCoord(x,y)) {
		if(sweeperMode==sweeperModeClassic &&
			lrQuickOpen && lbuttonDown)
		{
			quickOpen(x,y);
			checkWin();
		}
		else {
			int &tile=maskedMap(x,y);
			if(tile==tile_mask || tile==tile_flag) {
				tile^=tile_mask^tile_flag;
				drawScene();
			}
		}
	}
}
//
void MainWindow::onCreate() {
	hAccel=CreateAcceleratorTable(accelTable,ARRAYSIZE(accelTable));
    HMENU hMenu=CreateMenu(),hSubMenu;
    //游戏
    hSubMenu=CreatePopupMenu();
    AppendMenu(hSubMenu,MF_STRING,IDM_NEW_GAME,"新游戏\tN");
    // AppendMenu(hSubMenu,MF_STRING,IDM_CUSTOM_GAME,"自定义(&C)");
    AppendMenu(hSubMenu,MF_SEPARATOR,0,"NULL");
    AppendMenu(hSubMenu,MF_STRING|MF_CHECKED,IDM_CLASSIC_EASY,"简单\t1");
    AppendMenu(hSubMenu,MF_STRING,IDM_CLASSIC_NORMAL,"普通\t2");
    AppendMenu(hSubMenu,MF_STRING,IDM_CLASSIC_HARD,"困难\t3");
    AppendMenu(hSubMenu,MF_SEPARATOR,0,"NULL");
    AppendMenu(hSubMenu,MF_STRING,IDM_LOGIC_EASY,"无猜简单\t4");
    AppendMenu(hSubMenu,MF_STRING,IDM_LOGIC_NORMAL,"无猜普通\t5");
    AppendMenu(hSubMenu,MF_STRING,IDM_LOGIC_HARD,"无猜困难\t6");
    AppendMenu(hSubMenu,MF_SEPARATOR,0,"NULL");
    AppendMenu(hSubMenu,MF_STRING,IDM_EXIT,"退出(&X)\tAlt+F4");
    AppendMenu(hMenu,MF_POPUP,(UINT_PTR)hSubMenu,"游戏(&G)");
    //选项
    hSubMenu=CreatePopupMenu();
    AppendMenu(hSubMenu,MF_STRING,IDM_LEFT_QUICK_OPEN,"左键快速打开(&A)");
    AppendMenu(hSubMenu,MF_STRING,IDM_CLASSIC_NO_HELP,"无帮助(&0)");
    AppendMenu(hSubMenu,MF_STRING,IDM_CLASSIC_REFRESH_ONCE,"第一次点击保护(&1)");
    AppendMenu(hSubMenu,MF_STRING|MF_CHECKED,
				IDM_CLASSIC_REFRESH_EVERY,"常驻保护(&2)");
	AppendMenu(hSubMenu,MF_SEPARATOR,0,"NULL");
    AppendMenu(hSubMenu,MF_STRING|MF_DISABLED,
				IDM_LOGIC_ENTER_CHECK,"无猜按Enter检查(&B)");
    AppendMenu(hMenu,MF_POPUP,(UINT_PTR)hSubMenu,"选项(&S)");
    SetMenu(m_hwnd,hMenu);

	initGame(sweeperMode,gameDiffculty,false);
	m_created=true;
}
void MainWindow::onQuit() {
	m_ctx.release();
	DestroyAcceleratorTable(hAccel);
	hAccel=NULL;
}
void MainWindow::onSizing(int modifyType,RECT *rect) {
	int minw=300,minh=200;
	if(rect->right-rect->left<minw)rect->right=rect->left+minw;
	if(rect->bottom-rect->top<minh)rect->bottom=rect->top+minh;
}
void MainWindow::onSize(int flag,int w,int h) {
	if(!m_created)return;
	GetClientRect(m_hwnd,&m_cliRect);
	cout<<"client w,h: "<<w<<" "<<h<<endl;
	m_ctx.create(m_hwnd,w,h);
	// memset(m_ctx.pBuffer,0xEE,w*h*4);//清屏
	graphics::onSize(m_ctx,w,h);
	drawScene();
	SetForegroundWindow(m_hwnd);
}
void MainWindow::onPaint(HDC hdc) {
	flushBuffer();
}
void MainWindow::onTimer(int tid) {
	if(tid==IDT_OPENCELLS) {
		updateOpeningCells();
		if(openingCells.empty())
			KillTimer(m_hwnd,tid);
		drawScene();
	}
	else if(tid==IDT_BOOMANIM) {
		if(!boomAnim()) {
			KillTimer(m_hwnd,tid);
		}
	}
}
void MainWindow::onCommand(int cmd) {
	int mode=-1,diffc=0,helpLevel=-1;
	HMENU hmenu=GetMenu(m_hwnd);
	if(cmd==IDM_NEW_GAME)
		mode=sweeperMode,diffc=gameDiffculty;
	else if(cmd==IDM_EXIT) DestroyWindow(m_hwnd);
	else if(cmd==IDM_CLASSIC_EASY)
		mode=sweeperModeClassic,diffc=gameDiffcultyEasy;
	else if(cmd==IDM_CLASSIC_NORMAL)
		mode=sweeperModeClassic,diffc=gameDiffcultyNormal;
	else if(cmd==IDM_CLASSIC_HARD)
		mode=sweeperModeClassic,diffc=gameDiffcultyHard;
	else if(cmd==IDM_LOGIC_EASY)
		mode=sweeperModeLogic,diffc=gameDiffcultyEasy;
	else if(cmd==IDM_LOGIC_NORMAL)
		mode=sweeperModeLogic,diffc=gameDiffcultyNormal;
	else if(cmd==IDM_LOGIC_HARD)
		mode=sweeperModeLogic,diffc=gameDiffcultyHard;
	else if(cmd==IDM_LEFT_QUICK_OPEN) {
		lrQuickOpen^=1;
		CheckMenuItem(hmenu,IDM_LEFT_QUICK_OPEN,
						!lrQuickOpen?MF_CHECKED:MF_UNCHECKED);
	}
	else if(cmd==IDM_CLASSIC_NO_HELP) helpLevel=0;
	else if(cmd==IDM_CLASSIC_REFRESH_ONCE) helpLevel=1;
	else if(cmd==IDM_CLASSIC_REFRESH_EVERY) helpLevel=2;
	else if(cmd==IDM_LOGIC_ENTER_CHECK) {
		logicModeEnterCheck^=1;
		CheckMenuItem(hmenu,IDM_LOGIC_ENTER_CHECK,
						logicModeEnterCheck?MF_CHECKED:MF_UNCHECKED);
	}
	//
	if(helpLevel!=-1) {	//更改帮助等级
		classicModeHelpLevel=helpLevel;
		CheckMenuItem(hmenu,IDM_CLASSIC_NO_HELP,MF_UNCHECKED);
		CheckMenuItem(hmenu,IDM_CLASSIC_REFRESH_ONCE,MF_UNCHECKED);
		CheckMenuItem(hmenu,IDM_CLASSIC_REFRESH_EVERY,MF_UNCHECKED);
		CheckMenuItem(hmenu,cmd,MF_CHECKED);
	}
	if(mode!=-1) {	//开启新游戏
		HMENU hmenu=GetMenu(m_hwnd);
		CheckMenuItem(hmenu,IDM_CLASSIC_EASY,MF_UNCHECKED);
		CheckMenuItem(hmenu,IDM_CLASSIC_NORMAL,MF_UNCHECKED);
		CheckMenuItem(hmenu,IDM_CLASSIC_HARD,MF_UNCHECKED);
		CheckMenuItem(hmenu,IDM_LOGIC_EASY,MF_UNCHECKED);
		CheckMenuItem(hmenu,IDM_LOGIC_NORMAL,MF_UNCHECKED);
		CheckMenuItem(hmenu,IDM_LOGIC_HARD,MF_UNCHECKED);
		if(cmd!=IDM_NEW_GAME) CheckMenuItem(hmenu,cmd,MF_CHECKED);
		auto flag1=(mode==sweeperModeClassic)?MF_ENABLED:MF_DISABLED;
		auto flag2=flag1^MF_ENABLED^MF_DISABLED;
		EnableMenuItem(hmenu,IDM_LEFT_QUICK_OPEN,flag1);
		EnableMenuItem(hmenu,IDM_CLASSIC_NO_HELP,flag1);
		EnableMenuItem(hmenu,IDM_CLASSIC_REFRESH_ONCE,flag1);
		EnableMenuItem(hmenu,IDM_CLASSIC_REFRESH_EVERY,flag1);
		EnableMenuItem(hmenu,IDM_LOGIC_ENTER_CHECK,flag2);
		//
		int oldDiffc=gameDiffculty;
		initGame_core(mode,diffc,true);
		if(oldDiffc==diffc) {
			initGame(sweeperMode,gameDiffculty,false);
			drawScene();
		}
		else {	//需要更新窗口尺寸
			DWORD style=GetWindowLong(m_hwnd,GWL_STYLE);
			DWORD exStyle=GetWindowLong(m_hwnd,GWL_EXSTYLE);
			if(style&WS_MAXIMIZE) {
				initGame(sweeperMode,gameDiffculty,false);
				int w=m_cliRect.right-m_cliRect.left;
				int h=m_cliRect.bottom-m_cliRect.top;
				onSize(-1,w,h);
			}
			else {
				int w=graphics::calcWindowWidth(graphics::cellW);
				int h=graphics::calcWindowHeight(graphics::cellW);
				int scrw=graphics::getScreenW();
				int scrh=graphics::getScreenH();
				if(w>scrw*0.95 || h>scrh*0.95) {
					ShowWindow(m_hwnd,SW_MAXIMIZE);
				}
				else {
					RECT rc={0,0,w,h};
					AdjustWindowRectEx(&rc,style,FALSE,exStyle);
					SetWindowPos(
						m_hwnd,NULL,
						0,0,
						rc.right-rc.left,rc.bottom-rc.top,
						SWP_NOMOVE
					);
				}
				initGame(sweeperMode,gameDiffculty,false);
				drawScene();
			}
		}
	}
}
void MainWindow::onKeyDown(int vkcode) {
	// cout<<"WM_KEYDOWN"<<endl;
	if(vkcode==VK_ESCAPE) {
		DestroyWindow(m_hwnd);
	}
	else if(vkcode==VK_RETURN) {
		if(logicModeEnterCheck)checkWin(true);
	}
}
void MainWindow::onLButtonDown(int x,int y) {
	lbuttonDown=true;
	if(!blockAction) {
		if(sweeperMode==sweeperModeLogic)
			lclickCell(x,y);
		if(emojiState==emojiInit || emojiState==emojiPending) {
			emojiState=emojiPending;
			drawEmoji();
			emojiState=emojiInit;
		}
	}
}
void MainWindow::onRButtonDown(int x,int y) {
	rbuttonDown=true;
	if(!blockAction && sweeperMode==sweeperModeLogic)
		rclickCell(x,y);
}
void MainWindow::onLButtonUp(int x,int y) {
	lbuttonDown=false;
	int emx=graphics::emojiX,emy=graphics::emojiY;
	if(inBoard(x,y,emx,emy,emx+graphics::emojiW,emy+graphics::emojiW)) {
		initGame(sweeperMode,gameDiffculty,true);
		return drawScene();
	}
	if(blockAction || sweeperMode==sweeperModeLogic)
		return drawEmoji();
	if(stopOpeningAnim())flushBuffer();
	lclickCell(x,y);
	drawEmoji();
}
void MainWindow::onRButtonUp(int x,int y) {
	rbuttonDown=false;
	if(blockAction || sweeperMode==sweeperModeLogic)
		return;
	if(stopOpeningAnim())flushBuffer();
	rclickCell(x,y);
}

}	//end namespace wind



// int WINAPI WinMain(HINSTANCE hInstance
// 	,HINSTANCE hPrevInstance,PSTR pCmdLine,int nCmdShow)
int main()
{
	HINSTANCE hInstance=NULL; int nCmdShow=SW_SHOW;//
	msDelayOn();
	gdipOn();
	graphics::initResource();
	
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),10);
	initGame_core(0,0,true);
	graphics::initLayout(graphics::getDefCellW());
	gameGraphic::MainWindow window(hInstance,
		graphics::windowWidth,graphics::windowHeight,1);
	window.show(nCmdShow);
	
	MSG msg={};
	HWND hwnd=window.getHWnd();
	while(GetMessage(&msg,NULL,0,0)>0) {
		if(!TranslateAccelerator(hwnd,gameGraphic::hAccel,&msg)) {
	        TranslateMessage(&msg);
	        DispatchMessage(&msg);
	    }
	}

	graphics::releaseResource();
	gdipOff();
	msDelayOff();
    return msg.wParam;
}



































