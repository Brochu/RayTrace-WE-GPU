#pragma once

#include "Hittable.h"

#include <memory>
#include <vector>

using std::shared_ptr;
using std::make_shared;

class Hittable_list : public Hittable
{
public:
	Hittable_list() {}
	Hittable_list(shared_ptr<Hittable> obj) { add(obj); }

	void clear() { objects.clear(); }
	void add(shared_ptr<Hittable> obj) { objects.push_back(obj); }

	virtual bool hit(const Ray& r, double t_min, double t_max, hit_record& rec) const override;

	std::vector<shared_ptr<Hittable>> objects;
};

