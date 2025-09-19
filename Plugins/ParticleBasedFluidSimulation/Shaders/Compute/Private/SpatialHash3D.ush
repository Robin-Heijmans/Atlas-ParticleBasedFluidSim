static const int3 offsets3D[27] =
{
	int3(-1, -1, -1),
	int3(-1, -1, 0),
	int3(-1, -1, 1),
	int3(-1, 0, -1),
	int3(-1, 0, 0),
	int3(-1, 0, 1),
	int3(-1, 1, -1),
	int3(-1, 1, 0),
	int3(-1, 1, 1),
	int3(0, -1, -1),
	int3(0, -1, 0),
	int3(0, -1, 1),
	int3(0, 0, -1),
	int3(0, 0, 0),
	int3(0, 0, 1),
	int3(0, 1, -1),
	int3(0, 1, 0),
	int3(0, 1, 1),
	int3(1, -1, -1),
	int3(1, -1, 0),
	int3(1, -1, 1),
	int3(1, 0, -1),
	int3(1, 0, 0),
	int3(1, 0, 1),
	int3(1, 1, -1),
	int3(1, 1, 0),
	int3(1, 1, 1)
};

static const uint HashKey1 = 467;
static const uint HashKey2 = 1997;
static const uint HashKey3 = 57847;

uint3 PositionToCellCoords(float3 position, float radius) {
    return (int3)floor(position/radius);
}

uint HashCell(int3 cellCoords) {
    return cellCoords.x * HashKey1 + cellCoords.y * HashKey2 + cellCoords.z * HashKey3;
}

uint GetKeyFromHash(uint hash, uint tableSize) {
    return Hash % tableSize;
}