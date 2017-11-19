#include <iostream>
#include <vector>

using namespace std;

class Intersection;

class Road {
private:
	string name;
	int travelTime;
	vector<Intersection> endPoints;
	void setEndPoint(Intersection point);
public:
	Road(int time) {
		travelTime = time;
	}
};

class Intersection {
private:
	vector<Road*> roadsThatIntersect;
	string name;
	vector<int> roadDensity;
public:
	Intersection(string name, vector<Road*>roads, vector<int>density) {
		name = name;
		roadsThatIntersect = roads;
		roadDensity = density;
	}
};

void Road::setEndPoint(Intersection point) {
	endPoints.push_back(point);
}

class Map {
private:
	vector<Road> roads;
	vector<Intersection> intersections;
	int minimumLightTime;
	int maximumLightTime;
public:
	void importRoads(Road inputRoads) {
		roads.push_back(inputRoads);
	}

	void importIntersections(Intersection inputIntersections) {
		intersections.push_back(inputIntersections);
	}

	vector<vector<int>> lightOptimization() {
		vector<vector<int>> lightTimes;

		return lightTimes;
	}

	void setMin(int min) {
		minimumLightTime = min;
	}

	void setMax(int max) {
		maximumLightTime = max;
	}
};

void main() {
	Map map;
	map.setMin(15);
	map.setMax(30);

	map.lightOptimization();
}