#include <string>
#include <vector>
#include "utils.h"
#include <float.h>
#include "component_ai.h"

namespace SGame
{

CRange::CRange( AI* _owner, const char* _name ) :owner_(_owner), name_(_name)
{

}

CRange::~CRange()
{

}

bool CRange::is_in_circle( const VECTOR3& _p0, const VECTOR& _p1, float _r, float _r2 )
{
	int _x0 = _p0.x;
	int _z0 = _p0.z;
	int _x1 = _p1.x;
	int _z1 = _p1.z;

	if(_r == 0)
	{
		return false;
	}

	if(_x0 == _x1 && _z0 == _z1)
	{
		return true;
	}

	int dx = _x1 - _x0;
	int dz = _z1 - _z0;

	float nr = -_r;
	if(dx <= _r && dx >= nr && dz <= _r && dz >= nr)
	{
		return _r2 >= (dx * dx + dz * dz);
	}
	else
	{
		return false;
	}
}

bool CRange::is_in_sector( const VECTOR3& _forward, const VECTOR3& _p0, const VECTOR3& _p1, int _r, int _r2, float _angle )
{
	int x0 = _p0.x;
    int z0 = _p0.z;
	int x1 = _p1.x;
	int z1 = _p1.z;

	if(_r == 0)
	{
		return false;
	}

	if (x0 == x1 && z0 == z1)
	{
		return true;
	}

	if (!is_in_circle(_p0, _p1, _r, _r2))
	{
		return false;
	}

	int dx = x1 - x0;
	int dz = z1 - z0;

	VECTOR3 v(dx, 0, dz);

	VECTOR3 forward_(_forward);
	forward_.y = 0;

	return dot_angle(v, forward_) <= _angle;
}


SectorRange::SectorRange( AI* _owner, const char* _name, float _radius, float _buffer, float _angle, float _buffer_angle ) 
	: CircleRange(_owner, _name, _radius, _buffer), angle_(_angle), buffer_angle_(_buffer_angle)
{

}

bool SectorRange::is_in_range( const VECTOR3& _p ) const
{
	return is_in_sector(owner_->get_speed_vec(), owner_->get_pos(), _p, radius_,
		radius2_, angle_);
}

bool SectorRange::is_out_range( const VECTOR3& _p ) const
{
	return !is_in_sector(owner_->get_speed_vec(), owner_->get_pos(), _p, buffer_,
		buffer2_, buffer_angle_);
}


CircleRange::CircleRange( AI* _owner, const char* _name, float _radius, float _buffer ) 
	: CRange(_owner, _name)
{
	set_radius(_radius, _buffer);
}

bool CircleRange::is_out_range( const VECTOR3& _p ) const
{
	return !is_in_circle(owner_->get_pos(), _p, buffer_, buffer2_);
}

bool CircleRange::is_in_range( const VECTOR3& _p ) const
{
	return is_in_circle(owner_->get_pos(), _p, radius_, radius2_);
}


SectorCircleRange::SectorCircleRange( AI* _owner, const char* _name, const CircleRange _circle_range, const SectorRange _sector_range ) 
	: CRange(_owner, _name), circle_(_circle_range), sector_(_sector_range)
{

}

bool SectorCircleRange::is_in_range( const VECTOR3& _p ) const
{
	VECTOR3 pos;
	float rate;
	bool ignore_block = owner_->is_ignore_block() ? true : owner_->raycast(owner_->get_pos(), _p, pos, rate);
	return (circle_.is_in_range(_p) && ignore_block) || sector_.is_in_range(_p);
}

Tracker::Tracker() 
	:inited_(false)
{

}

void Tracker::init( TYPE _type, float _period )
{
    if( _period < 0 )
    {
        ERR(2)( "[Tracker](init)period: %f, must > 0", _period );
        return;
    }

	type_ = _type;
	set_period(_period);

	inited_ = true;
}

void Tracker::activate(uint32_t _now_t)
{
	if (!inited_)
	{
		return;
	}

	switch(type_)
	{
	case TYPE_RAND:
		return;
		// self.the_time = now_t + random.uniform(0, self.period)
		break;

	case TYPE_PERIOD:
		the_time_ = _now_t + period_;
		break;

	case TYPE_NOW:
		the_time_ = 0;
		break;

	default:
		return;
	}
}

void Tracker::delay(int32_t _delay_time)
{
	if(!inited_)
	{
		return;
	}
	if(type_ != TYPE_NOW)
	{
		period_ += _delay_time;
	}
	the_time_ += _delay_time;
}

bool Tracker::is_ready(uint32_t _now_t)
{
	if(!inited_)
	{
		return false;
	}
	if( _now_t >= the_time_ )
	{
		the_time_ = _now_t + period_;
		return true;
	}
	else
	{
		return false;
	}
}

void Tracker::set_period(float _period)
{
	_period *= THOUSAND;
	int32_t frame = g_timermng->get_ms_one_frame();
	_period = ceilf(_period/frame)*frame;
	period_ = static_cast<int32_t>(_period);
}

uint32_t Tracker::get_period() const
{
	return period_;
}


Smoother::Smoother()
{

}

const VECTOR3& Smoother::update( const VECTOR3& _recent )
{
	VECTOR3 recent = _recent;
	normalize2d(recent);
	old_ = history_[update_slot_];
	history_[update_slot_] = recent;
	update_slot_ += 1;

	if(update_slot_ == size_)
	{
		update_slot_ = 0;
	}
	summing_ = summing_ + (recent - old_);

	sum_ = summing_;
	sum_ = sum_ * recip_size_;

	normalize2d(sum_);
	return sum_;
}

void Smoother::init(int32_t _size)
{
	for (int32_t i = 0; i< _size; ++i)
	{
		history_.push_back(VECTOR3(0, 0, 0));
	}
	size_ = _size;
	recip_size_ = 1.0/size_;
	update_slot_ = 0;
	vc3_init(summing_, 0, 0, 0);
}

void Smoother::clear()
{
	for (int32_t i = 0; i< size_; ++i)
	{
		vc3_init(history_[i], 0, 0, 0);
	}
	update_slot_ = 0;
	vc3_init(summing_, 0, 0, 0);
}


float radians(float _degree)
{
	return PI * _degree / 180.f;
}

float degrees(float _radian)
{
	return 180.f * _radian / PI;
}

void rotate_vec_y_axis(VECTOR3* _vec, float _angle)
{
	_angle = radians(_angle);
	float cosa = cosf(_angle);
	float sina = sinf(_angle);
	float x = cosa * _vec->x + sina * _vec->z;
	float z = -sina* _vec->x + cosa * _vec->z;
	_vec->x = x;
	_vec->z = z;
}

float length2d(const VECTOR3& _vec)
{
	return sqrt(_vec.x * _vec.x + _vec.z * _vec.z);
}

bool further2d(const VECTOR3& _vec, float _dist)
{
	return _vec.x * _vec.x + _vec.z * _vec.z > _dist * _dist;
}

void normalize2d( VECTOR3& _vec, float _length /*= 1.0*/ )
{
	_vec.y = 0;
	if (!vc3_iszero(_vec))
	{
		vc3_norm(_vec, _vec, _length);
	}
}


float clamp_dot(float _dot)
{
	if(_dot < -1)
	{
		_dot = -1;
	}
	else if (_dot > 1)
	{
		_dot = 1;
	}
	return _dot;
}

int vc2_iszero( const VECTOR3& _src1 )
{
	if (_src1.x > FLT_EPSILON || _src1.x < -FLT_EPSILON)
	{
		return 0;
	}
	if (_src1.z > FLT_EPSILON || _src1.z < -FLT_EPSILON)
	{
		return 0;
	}
	return 1;
}

float dot_angle(const VECTOR3& _a, const VECTOR3& _b)
{
	VECTOR3 na(_a);
	VECTOR3 nb(_b);

	if (!vc3_iszero(na))
	{
		vc3_norm(na, na);
	}

	if (!vc3_iszero(nb))
	{
		vc3_norm(nb, nb);
	}

	return acos(clamp_dot(vc3_dot(na, nb)));
}

float dir2radian(const VECTOR3& _dir)
{
	VECTOR3 dir(_dir);
	dir.y = 0;
	float radian = dot_angle(dir, VECTOR3(0, 0, 1));
	if(dir.x < 0)
	{
		radian = 2 * PI - radian;
	}
	return radian;
}

string& str_trim_bracket(string* _str)
{
	// trim left
	_str->erase(0, _str->find_first_not_of("("));
	// trim right
	_str->erase(_str->find_last_not_of(")")+1);

	// trim left
	_str->erase(0, _str->find_first_not_of("["));
	// trim right
	_str->erase(_str->find_last_not_of("]")+1);

	// trim left
	_str->erase(0, _str->find_first_not_of("{"));
	// trim right
	_str->erase(_str->find_last_not_of("}")+1);
	return *_str;
}

string& str_trim(string* _str, string _sep /*= " "*/)
{
	// trim right
	_str->erase(_str->find_last_not_of(_sep)+1);
	// trim left
	_str->erase(0, _str->find_first_not_of(_sep));
	return *_str;
}

string& str_rep(string& _str, string _sep, string _rep)//12
{
	 int begin = 0;
	 begin = _str.find(_sep, begin);
	 while(begin != -1)
	 {  
		_str.replace(begin, 1, _rep);
		begin = _str.find(_sep, begin);
	 }
	 return _str;
}

void str_split(const string& _str, vector<string>& _ret, string _sep /*= ","*/)
{
	_ret.clear();
	if (_str.empty())
	{
		return;
	}
	string tmp;
	string::size_type pos_begin = _str.find_first_not_of(_sep);
	string::size_type comma_pos = 0;
	while (pos_begin != string::npos)
	{
		comma_pos = _str.find(_sep, pos_begin);
		if (comma_pos != string::npos)
		{
			tmp = _str.substr(pos_begin, comma_pos - pos_begin);
			pos_begin = comma_pos + _sep.length();
		}
		else
		{
			tmp = _str.substr(pos_begin);
			pos_begin = comma_pos;
		}

		if (!tmp.empty())
		{
			_ret.push_back(tmp);
			tmp.clear();
		}
	}
}

float rand_float(float _min, float _max)
{
	// gen a random float number in [min, max)
	return _min + (_max - _min) * (static_cast<float>(rand() % RAND_MAX) / RAND_MAX);
}

int rand_int(int _min, int _max)
{
    if( _min > _max )
    {
        _min = _max;
    }
	// rand() returns [0, RAND_MAX], RAND_MAX is is 2147483647(INT_MAX) in GNU library
	// gen a random int number in [min, max]
	return rand() % ((_max - _min) + 1) + _min;
}

float norm_angle(float _angle)
{
    if( _angle < 0 )
    {
        _angle = fmod( _angle, 360.0f ) + 360.0f;
    }

    _angle = fmod( _angle, 360.0f );

	return _angle;
}

}

