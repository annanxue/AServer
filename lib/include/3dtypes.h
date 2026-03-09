#ifndef __TYPE_H__
#define __TYPE_H__

struct Point
{
	long x;
	long y;
	Point() {}
	Point( long _x, long _y ):x(_x),y(_y) {}
};

struct Rect
{
	long top;
	long left;
	long bottom;
	long right;

	Rect() {}
	Rect( long _top, long _left, long _bottom, long _right):top(_top),left(_left),bottom(_bottom),right(_right) {}
	Rect( Point _tl, Point _br ):top(_tl.y),left(_tl.x),bottom(_br.y),right(_br.x) {}
	void SetRect( long _left, long _top, long _right, long _bottom ) { top=_top;left=_left;bottom=_bottom;right=_right; }
	void SetRect( Point _tl, Point _br ) { top=_tl.y;left=_tl.x;bottom=_br.y;right=_br.x; }
	Point top_left() { return Point( left, top ); }
	Point bottom_right() { return Point( right, bottom ); }
	int PtInRect( Point pt ) { return ((pt.x > left) && (pt.x < right) && (pt.y < bottom) && (pt.y > top)); }
};


#endif
