
#ifndef __NAVMESH_H__
#define __NAVMESH_H__

#include <map>
#include <vector>
#include <string>

#include "DetourCommon.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshQuery.h"
#include "vector.h"
#include "log.h"

using namespace Sgame;

class Navmesh
{
public:
	explicit Navmesh(const char* _nav_path);
	~Navmesh();

	typedef std::vector<VECTOR3> Path;

	bool findpath(const VECTOR3* _start, const VECTOR3* _end, Path* _path_point, unsigned short _door_flag) const;
	bool raycast(const VECTOR3* _start, const VECTOR3* _end, VECTOR3* _hit_pos, float* _rate, unsigned short _door_flag) const;
	bool find_pos_to_wall(const VECTOR3* _pos, float _radius, VECTOR3* _hit_pos, unsigned short _door_flag) const;

    bool get_valid_pos( const VECTOR3* _pos, float _radius, VECTOR3* _result_pos, unsigned short _door_flag ) const;

	bool is_same_type_flag(const char *_type_flag) const;
	const char* get_type_flag() const;

	static void destroy_all_navmesh();

	bool is_good() const
	{
		return initialized_;
	}

private:
	dtNavMesh* new_navmesh(const char* _path);
	dtNavMesh* get_navmesh();
	bool read_tile(FILE *fp, dtNavMesh *_mesh);
	dtNavMeshQuery* get_navquery();

    bool get_poly_on_boundary(const VECTOR3* _pos, dtQueryFilter* _filter, dtPolyRef* _poly_ref) const;

	struct NavMeshSetHeader
	{
		//int magic;
		int version;
		int numTiles;
		dtNavMeshParams params;
	};

	struct NavMeshTileHeader
	{
		dtTileRef tileRef;
		int dataSize;
	};

	typedef std::map<std::string, dtNavMesh*> NavmeshMap;
	typedef std::map<std::string, dtNavMeshQuery*> NavqueryMap;

	static NavqueryMap g_navquery_map;
	static NavmeshMap g_navmesh_map;

	static const int NAVMESHSET_MAGIC = 'M'<<24 | 'S'<<16 | 'E'<<8 | 'T';  // 'MSET';
	static const int NAVMESHSET_VERSION = 1;

	static const int MAX_POLYS = 256;
	static float g_straight_path[MAX_POLYS*3];
	static dtPolyRef g_polys[MAX_POLYS];

	dtNavMeshQuery *query_;
	dtNavMesh *mesh_;

	std::string type_flag_;
	bool initialized_;
};

#endif  // #ifndef __NAVMESH_H__
