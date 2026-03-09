#ifndef __UTILS_H__
#define __UTILS_H__

#include <cmath>
#include <string>
#include <vector>
#include "common.h"

namespace SGame
{

class AI;

class CRange
{
public:
	CRange(AI* _owner, const char* _name);
	virtual ~CRange();
	virtual bool is_in_range(const VECTOR3& _p) const = 0;
	virtual bool is_out_range(const VECTOR3& _p) const = 0;
	static bool is_in_circle(const VECTOR3& _p0, const VECTOR& _p1, float _r, float _r2);
	static bool is_in_sector(const VECTOR3& _forward, const VECTOR3& _p0, const VECTOR3& _p1, int _r, int _r2, float _angle);

protected:
	AI* owner_;

private:
	const char* name_;
};

class CircleRange : public CRange
{
public:
	CircleRange(AI* _owner, const char* _name, float _radius, float _buffer);
	bool is_in_range(const VECTOR3& _p) const;
	bool is_out_range(const VECTOR3& _p) const;
	float get_radius() const;

private:
	void set_radius(float _radius, float _buffer);

protected:
	float radius_;
	float buffer_;
	float radius2_;
	float buffer2_;
};

inline float CircleRange::get_radius() const
{
	return radius_;
}

inline void CircleRange::set_radius(float _radius, float _buffer)
{
	radius_ = _radius;
	buffer_ = _buffer;
	radius2_ = _radius * _radius;
	buffer2_ = _buffer * _buffer;
}


class SectorRange : public CircleRange
{
public:
	SectorRange(AI* _owner, const char* _name, float _radius, float _buffer, float _angle, float _buffer_angle);
	bool is_in_range(const VECTOR3& _p) const;
	bool is_out_range(const VECTOR3& _p) const;

private:
	float angle_;
	float buffer_angle_;
};


class SectorCircleRange : public CRange
{
public:
	SectorCircleRange(AI* _owner, const char* _name, const CircleRange _circle_range, const SectorRange _sector_range);
	bool is_in_range(const VECTOR3& _p) const;
	bool is_out_range(const VECTOR3& _p) const;


private:
	CircleRange circle_;
	SectorRange sector_;
};

inline bool SectorCircleRange::is_out_range( const VECTOR3& _p ) const
{
	return circle_.is_out_range(_p) && sector_.is_out_range(_p);
}


class Tracker
{
public:
	enum TYPE
	{
		TYPE_RAND,
		TYPE_PERIOD,
		TYPE_NOW,
	};

	Tracker();
	void init(TYPE _type, float _period = 0.f);
	void activate(uint32_t _now_t);
	void delay(int32_t _delay_time);
	bool is_ready(uint32_t _now_t);
	void set_period(float _period);
	uint32_t get_period() const;

public:
	int32_t period_;
	uint32_t the_time_;

private:
	TYPE type_;
	bool inited_;
};


class Smoother
{
public:
	Smoother();
	void init(int32_t _size);
	const VECTOR3& update(const VECTOR3& _recent);
	void clear();

private:
	int32_t size_;
	float recip_size_;
	int32_t update_slot_;
	VECTOR3 summing_;
	VECTOR3 old_;
	VECTOR3 sum_;
	std::vector<VECTOR3> history_;
};

//other functions in file
float radians(float _degree);
float degrees(float _radian);
void rotate_vec_y_axis(VECTOR3* _vec, float _angle);
void normalize2d(VECTOR3& _vec, float _length = 1.0);
float length2d(const VECTOR3& _vec);
float dot_angle(const VECTOR3& _a, const VECTOR3& _b);
float clamp_dot(float _dot);
float dir2radian(const VECTOR3& _dir);
string& str_trim_bracket(string* _str);
string& str_trim(string* _str, string _sep = " ");
void str_split(const string& _str, vector<string>& _ret, string _sep = ",");
float rand_float(float _min, float _max);
int rand_int(int _min, int _max);
string& str_rep(string& _str, string _sep = " ", string _rep = "");
float norm_angle(float _angle);
int vc2_iszero( const VECTOR3& _src1 );

}

#endif  // __UTILS_H__
