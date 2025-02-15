#pragma once

#include <math.h>


constexpr float PI = 3.14159265f;
constexpr double PI_D = 3.1415926535897932;


template <typename T>
constexpr auto sq(const T& x) {
	return x * x;
}

template<typename T>
T wrap_angle(T theta) {
	const T modded = fmod(theta, static_cast<T>(2.0) * static_cast<T>(PI_D));
	return (modded > static_cast<T>(PI_D)) ?
		(modded - static_cast<T>(2.0) * static_cast<T>(PI_D)) :
		modded;
}

template<typename T>
constexpr T interpolate(const T& src, const T& dst, float alpha) {
	return src + (dst - src) * alpha;
}

template<typename T>
constexpr T to_rad(T deg) {
	return deg * PI / static_cast<T>(180.0);
}
