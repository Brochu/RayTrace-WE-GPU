#include "Hittable_list.h"

bool Hittable_list::hit(const Ray& r, double t_min, double t_max, hit_record& rec) const
{
	hit_record temp_rec;
	bool has_hit = false;
	auto closest_t = t_max;

	for (const auto& o : objects)
	{
		if (o->hit(r, t_min, closest_t, temp_rec))
		{
			has_hit = true;
			closest_t = temp_rec.t;
			rec = temp_rec;
		}
	}

	return has_hit;
}
