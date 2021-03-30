#pragma once

#include "Hittable.h"
#include "Vec3.h"

class Sphere : public Hittable
{
public:
	Sphere() {};
	Sphere(Point3 cen, double r) : center(cen), radius(r) {};

	virtual bool hit(const Ray& r, double t_min, double t_max, hit_record& rec) const override;

	Point3 center;
	double radius;
};
