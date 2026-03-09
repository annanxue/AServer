
#include <cmath>
#include <cstdio>
#include <cfloat>
#include <cstring>
#include <map>
#include <string>
#include <vector>
#include <algorithm>

#include "navmesh.h"
#include "3dmath.h"

#define VALID_CHECK_ANGLE 30
#define VALID_ADD_ANGLE 48
#define NEARESTPOLYDIST 0.3

using std::max;

Navmesh::NavqueryMap Navmesh::g_navquery_map;
Navmesh::NavmeshMap Navmesh::g_navmesh_map;
float Navmesh::g_straight_path[Navmesh::MAX_POLYS*3];
dtPolyRef Navmesh::g_polys[Navmesh::MAX_POLYS];

// only search y, from -1000 to 1000
static const float query_box[] = { 0, 1000, 0 };

Navmesh::Navmesh( const char* _nav_path ) : initialized_(false)
{
	type_flag_ = _nav_path; //todo: hash opt

	mesh_ = get_navmesh();
	if (mesh_ == NULL)
	{
		mesh_ = new_navmesh(_nav_path);
		if (NULL == mesh_)
		{
			ERR(2)("navmesh error, mesh init error, file=%s", _nav_path);
			return;
		}
		g_navmesh_map.insert(std::make_pair(type_flag_, mesh_));

		query_ = dtAllocNavMeshQuery();
		query_->init(mesh_, MAX_POLYS);

		g_navquery_map.insert(std::make_pair(type_flag_, query_));
	}
	else
	{
		query_ = get_navquery();
	}
	if(!query_ || !mesh_)
	{
		ERR(2)("navmesh init error,file=%s", _nav_path);
		return;
	}

	initialized_ = true;
}

Navmesh::~Navmesh()
{
}

dtNavMeshQuery* Navmesh::get_navquery()
{
	NavqueryMap::iterator iter = g_navquery_map.find(type_flag_);

	if (iter != g_navquery_map.end())
	{
		return iter->second;
	}

	return NULL;
}

dtNavMesh* Navmesh::get_navmesh()
{
	NavmeshMap::iterator iter = g_navmesh_map.find(type_flag_);

	if (iter != g_navmesh_map.end())
	{
		return iter->second;
	}

	return NULL;
}

dtNavMesh* Navmesh::new_navmesh( const char* _path )
{
	FILE* fp = fopen(_path, "rb");

	if(!fp)
	{
		ERR(2)("navmesh error, file can't open, file=%s", _path);
		return NULL;
	}

	NavMeshSetHeader header;
	fread(&header, sizeof(NavMeshSetHeader), 1, fp);

	if (header.version != NAVMESHSET_VERSION)
	{
		ERR(2)("navmesh error, old format file, file=%s", _path);
		fclose(fp);
		return NULL;
	}

	dtNavMesh* mesh = dtAllocNavMesh();
	if (!mesh || mesh->init(&header.params) != DT_SUCCESS)
	{
		ERR(2)("navmesh error, bad params, file=%s", _path);
		fclose(fp);
		return mesh;
	}
	
	for (int i = 0; i < header.numTiles; ++i)
	{
		if(!read_tile(fp, mesh))
		{
			ERR(2)("navmesh error, read tile error, file=%s", _path);
			fclose(fp);
			return mesh;
		}
	}
	fclose(fp);
	return mesh;
}

bool Navmesh::get_poly_on_boundary(const VECTOR3* _pos, dtQueryFilter* _filter, dtPolyRef* _poly_ref) const
{

	VECTOR3 closest_pos;

    if( query_->findNearestPoly(&_pos->x, &query_box[0], _filter, _poly_ref, &closest_pos.x) == DT_SUCCESS )
    {
        return (*_poly_ref) != 0;
    }

    return false;
}

bool Navmesh::findpath( const VECTOR3* _start_pos, const VECTOR3* _end_pos, Path* _path_point, unsigned short _door_flag ) const
{
	dtPolyRef start_poly = 0, end_poly = 0;

	dtQueryFilter filter;
	filter.setIncludeFlags(_door_flag);

	if( !get_poly_on_boundary( _start_pos, &filter, &start_poly ) || !get_poly_on_boundary( _end_pos, &filter, &end_poly ) )
	{
		return false;
	}

	int npolys = 0;
	query_->findPath( start_poly, end_poly, &_start_pos->x, &_end_pos->x, &filter, g_polys, &npolys, MAX_POLYS );

	if (!npolys)
	{
		return false;
	}

	int path_len = 0;
	query_->findStraightPath(&_start_pos->x, &_end_pos->x, g_polys, npolys, g_straight_path,
		NULL,  NULL, &path_len, MAX_POLYS);

	if (path_len == 2) 
	{
		VECTOR3 hit_pos;
		float hit_rate;
		raycast( _start_pos, _end_pos, &hit_pos, &hit_rate, _door_flag );
		_path_point->push_back(VECTOR3(*_start_pos));
		_path_point->push_back(VECTOR3(hit_pos));
	}
	else
	{
		for (int i = 0; i < path_len; ++i)
		{
			_path_point->push_back(VECTOR3(g_straight_path[i*3],
				g_straight_path[i*3+1], g_straight_path[i*3+2]));
		}
	}

	if(path_len > 1)
	{
		return true;
	}

	return false;
}

bool Navmesh::raycast( const VECTOR3* _start_pos, const VECTOR3* _end, VECTOR3* _hit_pos, float* _rate, unsigned short _door_flag ) const
{
	dtPolyRef start_poly = 0;
	VECTOR3 end_pos(*_end);

	*_hit_pos = *_start_pos;
	*_rate = 0.f;

	//LOG(2) ("------------------raycast: %d", _door_flag);
	dtQueryFilter filter;
	filter.setIncludeFlags(_door_flag);

    if( !get_poly_on_boundary( _start_pos, &filter, &start_poly ) )
    {
        return false;
    }

	VECTOR3 hit_normal;
	float t = 0.0;

	end_pos.y = _start_pos->y;

	int npolys = 0;

	if(DT_SUCCESS != query_->raycast(start_poly, &_start_pos->x, &end_pos.x, &filter, &t,
		&hit_normal.x, g_polys, &npolys, MAX_POLYS))
	{
		return false;
	}

	if (t >= 1.0f)
	{
		_hit_pos->x = end_pos.x;
		_hit_pos->y = end_pos.y;
		_hit_pos->z = end_pos.z;
	}
	else
	{
		_hit_pos->x = _start_pos->x + (end_pos.x - _start_pos->x) * t;
		_hit_pos->y = _start_pos->y + (end_pos.y - _start_pos->y) * t;
		_hit_pos->z = _start_pos->z + (end_pos.z - _start_pos->z) * t;
		if (npolys)
		{
			float h = 0;
			query_->getPolyHeight(g_polys[npolys-1], &_hit_pos->x, &h);
			_hit_pos->y = h;
		}
	}

	t = t > 1.0f ? 1.0f : t;
	*_rate = t;
	if(t >= 1.0f)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool Navmesh::find_pos_to_wall(const VECTOR3* _pos, float _radius, VECTOR3* _hit_pos, unsigned short _door_flag) const
{
	dtPolyRef start_poly = 0;

	dtQueryFilter filter;
	filter.setIncludeFlags(_door_flag);

    if( !get_poly_on_boundary( _pos, &filter, &start_poly ) )
    {
		return false;
    }

    float hit_dist;
    VECTOR3 hit_normal;
	if(DT_SUCCESS != query_->findDistanceToWall(start_poly, &_pos->x, _radius, &filter, &hit_dist, &(_hit_pos->x), &hit_normal.x) )
    {
        return false;
    }

    return true;
}

bool Navmesh::get_valid_pos( const VECTOR3* _pos, float _radius, VECTOR3* _result_pos, unsigned short _door_flag ) const
{
	dtQueryFilter filter;
    filter.setIncludeFlags( _door_flag );
	VECTOR3 closest_pos;

    if( query_->findInsidePolyPoint( &_pos->x, &query_box[0], &filter, NEARESTPOLYDIST, &closest_pos.x ) == DT_SUCCESS ) 
    {
        _result_pos->x = closest_pos.x;
        _result_pos->z = closest_pos.z;
        return true;
    }
    
    float angle = VALID_CHECK_ANGLE;  //选择从30度开始遍历
    const float add_angle = VALID_ADD_ANGLE;
	//VECTOR3 hit_pos;
	//float hit_rate = 0;
    for( int index = 0; index < 8; ++ index )
    {
        angle  +=  add_angle * index;
        VECTOR3 end_pos = GET_DIR( angle ) * _radius + *_pos;
        //hit_rate = 0;
        if( query_->findInsidePolyPoint( &end_pos.x, &query_box[0], &filter, NEARESTPOLYDIST, &closest_pos.x ) == DT_SUCCESS ) 
        {
            _result_pos->x = closest_pos.x;
            _result_pos->z = closest_pos.z;
            return true;
        }
    }
    return false;
}

bool Navmesh::is_same_type_flag( const char *_type_flag ) const
{
	if(type_flag_.compare(_type_flag) == 0)
	{
		return true;
	}
	return false;
}

const char* Navmesh::get_type_flag() const
{
	return type_flag_.c_str();
}

bool Navmesh::read_tile( FILE *fp, dtNavMesh *_mesh )
{
	NavMeshTileHeader tileHeader;
	fread(&tileHeader, sizeof(tileHeader), 1, fp);
	if (!tileHeader.tileRef || !tileHeader.dataSize)
	{
		return false;
	}
	unsigned char* data = (unsigned char*)dtAlloc(tileHeader.dataSize, DT_ALLOC_PERM);
	if (!data)
	{
		return false;
	} 
    memset(data, 0, static_cast<unsigned int>(tileHeader.dataSize));
	fread(data, static_cast<unsigned int>(tileHeader.dataSize), 1, fp);

	_mesh->addTile(data, tileHeader.dataSize, DT_TILE_FREE_DATA, tileHeader.tileRef, 0);

	return true;
}

void Navmesh::destroy_all_navmesh()
{
	for (Navmesh::NavmeshMap::iterator it = Navmesh::g_navmesh_map.begin();
			it != Navmesh::g_navmesh_map.end(); ++it)
	{
		dtFreeNavMesh(it->second);
	}
	for (Navmesh::NavqueryMap::iterator it = Navmesh::g_navquery_map.begin();
			it != Navmesh::g_navquery_map.end(); ++it)
	{
		dtFreeNavMeshQuery(it->second);
	}
}

