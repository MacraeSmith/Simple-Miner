#pragma once
#include "MathUtils.hpp"
#include <vector>


template<typename T>
class Curve
{
public:
	explicit Curve(T const& start, T const& end)
		:m_start(start), m_end(end) {}

	virtual ~Curve() = default;

	virtual T EvaluateAt(float t) const = 0;

	T m_start;
	T m_end;
};

template<typename T>
struct CurveWithTStart
{
	CurveWithTStart(Curve<T>* curve, float tStart)
		:m_curve(curve)
		,m_tStart(tStart) {}

	Curve<T>* m_curve = nullptr;
	float m_tStart = 0.f;
};

template<typename T>
class PieceWiseCurve : public Curve<T>
{
public:
	PieceWiseCurve(std::vector<CurveWithTStart<T>> const& curves, float tEnd)
		:Curve<T>(curves.empty() ? T{} : curves.front().m_curve->m_start,
			curves.empty() ? T{} : curves.back().m_curve->m_end)
			,m_curves(curves)
			,m_tEnd(tEnd){}

	~PieceWiseCurve()
	{
		for (int i = 0; i < (int)m_curves.size(); ++i)
		{
			delete m_curves[i].m_curve;
			m_curves[i].m_curve = nullptr;
		}
	}

	virtual T EvaluateAt(float t) const override
	{
		if (m_curves.empty())
			return this->m_start;

		if(t <= m_curves.front().m_tStart)
			return this->m_start;

		if(t >= m_tEnd)
			return this->m_end;

		if(m_curves.size() == 1)
			return m_curves[0].m_curve->EvaluateAt(t);

		const int NUM_CURVES = (int)m_curves.size();

		for (int i = 0; i < NUM_CURVES; ++i)
		{
			float tStart = m_curves[i].m_tStart;
			float tEnd = (i < NUM_CURVES - 1) ? m_curves[i + 1].m_tStart : m_tEnd;

			if (t >= tStart && t <= tEnd)
			{
				float fraction = GetClampedFractionWithinRange(t, tStart, tEnd);
				return m_curves[i].m_curve->EvaluateAt(fraction);
			}
		}

		return this->m_end;
	}

	void AddCurve(Curve<T>* curve)
	{
		m_curves.push_back(curve);
		this->m_end = curve->m_end;
	}

	void AlignCurves()
	{
		if (m_curves.empty())
			return;

		for (int i = 1; i < (int)m_curves.size(); ++i)
		{
			m_curves[i].m_curve->m_start = m_curves[i - 1].m_curve->m_end;
		}
	}

	std::vector<CurveWithTStart<T>> GetCurves() const {return m_curves;}

	std::vector<CurveWithTStart<T>> m_curves;
	float m_tEnd = 1.f;
};

template<typename T>
class LinearCurve : public Curve<T>
{
public:
	LinearCurve(T const& start, T const& end)
		:Curve<T>(start, end) {}

	virtual T EvaluateAt(float t) const override
	{
		return Lerp(this->m_start, this->m_end, t);
	}
};

template<typename T>
class SmoothStep3Curve : public Curve<T>
{
public:
	SmoothStep3Curve(T const& start, T const& end)
		:Curve<T>(start, end) {}

	virtual T EvaluateAt(float t) const override
	{
		float s = t * t * (3.f - (2.f * t));
		return Lerp(this->m_start, this->m_end, s);
	}
};

template<typename T>
class SmoothStep5Curve : public Curve<T>
{
public:
	SmoothStep5Curve(T const& start, T const& end)
		:Curve<T>(start, end) {}

	virtual T EvaluateAt(float t) const override
	{
		float s = t * t * t * (t * ((6.f * t) - 15.f) + 10.f);
		return Lerp(this->m_start, this->m_end, s);
	}
};

template<typename T>
class SmoothStartCurve : public Curve<T>
{
public:
	SmoothStartCurve(T const& start, T const& end, int exponent)
		:Curve<T>(start, end), m_exponent(exponent) {}

	virtual T EvaluateAt(float t) const override
	{
		float alteredT = t;
		for (int i = 0; i < m_exponent; ++i)
		{
			alteredT *= t;
		}
		return Lerp(this->m_start, this->m_end, alteredT);
	}

	int m_exponent = 1;
};

template<typename T>
using EaseInCurve = SmoothStartCurve<T>;

template<typename T>
class SmoothStopCurve : public Curve<T>
{
public:
	SmoothStopCurve(T const& start, T const& end, int exponent)
		:Curve<T>(start, end), m_exponent(exponent) {}

	virtual T EvaluateAt(float t) const override
	{
		float s = 1.f - t;
		for (int i = 0; i < m_exponent; ++i)
		{
			s *= s;
		}

		return Lerp(this->m_start, this->m_end, 1.f - s);
	}

	int m_exponent = 1;
};

template<typename T>
using EaseOutCurve = SmoothStopCurve<T>;

template<typename T>
class Hesitate3Curve : public Curve<T>
{
public:
	Hesitate3Curve(T const& start, T const& end)
		:Curve<T>(start, end) {}

	virtual T EvaluateAt(float t) const override
	{
		float s = 1 - t;
		float alteredT = (3.f * ((s * s) * t)) + (t * t * t);
		return Lerp(this->m_start, this->m_end, alteredT);
	}
};

template<typename T>
class Hesitate5Curve : public Curve<T>
{
public:
	Hesitate5Curve(T const& start, T const& end)
		:Curve<T>(start, end) {}

	virtual T EvaluateAt(float t) const override
	{
		float s = 1 - t;
		float alteredT = (5.f * t * (s * s * s * s)) + (10.f * (t * t * t) * (s * s)) + (t * t * t * t * t);
		return Lerp(this->m_start, this->m_end, alteredT);
	}
};

template<typename T>
class EaseInBackCurve : public Curve<T>
{
public:
	EaseInBackCurve(T const& start, T const& end)
		:Curve<T>(start, end) {}

	virtual T EvaluateAt(float t) const override
	{
		constexpr float c1 = 1.70158f;
		float c3 = c1 + 1.f;
		float alteredT = (c3 * t * t * t) - (c1 * t * t);
		return Lerp(this->m_start, this->m_end, alteredT);
	}

};

template<typename T>
class EaseOutBackCurve : public Curve<T>
{
public:
	EaseOutBackCurve(T const& start, T const& end)
		:Curve<T>(start, end) {}

	virtual T EvaluateAt(float t) const override
	{
		constexpr float c1 = 1.70158f;
		float c3 = c1 + 1.f;
		float x = t - 1.f;
		float alteredT = (1.f + c3) * (x * x * x) + (c1 * (x * x));
		return Lerp(this->m_start, this->m_end, alteredT);
	}

};





