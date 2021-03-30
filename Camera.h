#pragma once

#include "Vec3.h"
#include "Ray.h"

class Camera
{
public:
    Camera(unsigned int width, unsigned int height)
    {
		const auto aspect_ratio = (double)width / height;

		auto viewport_height = 2.0;
		auto viewport_width = aspect_ratio * viewport_height;
		auto focal_length = 1.0;

		origin = Vec3(0, 0, 0);
		horizontal = Vec3(viewport_width, 0, 0);
		vertical = Vec3(0, viewport_height, 0);
		lower_left_corner = origin - horizontal/2 - vertical/2 - Vec3(0, 0, focal_length);
    }

	Ray get_ray(double u, double v) const
	{
		return Ray(origin, lower_left_corner + u * horizontal + v * vertical - origin);
	}

private:
	Point3 origin;
	Vec3 horizontal;
	Vec3 vertical;
	Point3 lower_left_corner;
};
