#include "Sphere.h"

bool Sphere::hit(const Ray& r, double t_min, double t_max, hit_record& rec) const
{
	// Sphere intersection check
	Vec3 oc = r.origin() - center;

	auto a = r.direction().length_squared();
	auto half_b = dot(oc, r.direction());
	auto c = oc.length_squared() - radius*radius;

	auto discr = half_b*half_b - a*c;
	if (discr < 0) return false;

	auto sqrtd = sqrt(discr);
	auto root = (-half_b - sqrtd) / a;
	if (root < t_min || root > t_max)
	{
		root = (-half_b + sqrtd) / a;
		if (root < t_min || root > t_max)
		{
			return false;
		}
	}

	rec.t = root;
	rec.p = r.at(rec.t);
	Vec3 outward_normal = (rec.p - center) / radius;
	rec.set_face_normal(r, outward_normal);

	return true;
}
