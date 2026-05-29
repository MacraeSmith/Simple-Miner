#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Vec4.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Math/EulerAngles.hpp"

const Mat44 Mat44::IFWRD_JLEFT_KUP_TO_DX11RENDER = Mat44(Mat44(Vec4(0.f, 0.f, 1.f, 0.f), Vec4(-1.f, 0.f, 0.f, 0.f), Vec4(0.f, 1.f, 0.f, 0.f), Vec4(0.f, 0.f, 0.f, 1.f)));
const Mat44 Mat44::IDENTITY = Mat44(Vec4(1.f, 0.f, 0.f, 0.f), Vec4(0.f, 1.f, 0.f, 0.f), Vec4(0.f, 0.f, 1.f, 0.f), Vec4(0.f, 0.f, 0.f, 1.f));
Mat44::Mat44()
{
	m_values[Ix] = 1.f;
	m_values[Iy] = 0.f;
	m_values[Iz] = 0.f;
	m_values[Iw] = 0.f;

	m_values[Jx] = 0.f;
	m_values[Jy] = 1.f;
	m_values[Jz] = 0.f;
	m_values[Jw] = 0.f;

	m_values[Kx] = 0.f;
	m_values[Ky] = 0.f;
	m_values[Kz] = 1.f;
	m_values[Kw] = 0.f;

	m_values[Tx] = 0.f;
	m_values[Ty] = 0.f;
	m_values[Tz] = 0.f;
	m_values[Tw] = 1.f;
}

Mat44::Mat44(Vec2 const& iBasis2D, Vec2 const& jBasis2D, Vec2 const& translation2D)
{
	m_values[Ix] = iBasis2D.x;
	m_values[Iy] = iBasis2D.y;

	m_values[Jx] = jBasis2D.x;
	m_values[Jy] = jBasis2D.y;

	m_values[Tx] = translation2D.x;
	m_values[Ty] = translation2D.y;
}

Mat44::Mat44(Vec3 const& iBasis3D, Vec3 const& jBasis3D, Vec3 const& kBasis3D, Vec3 const& translation3D)
{
	m_values[Ix] = iBasis3D.x;
	m_values[Iy] = iBasis3D.y;
	m_values[Iz] = iBasis3D.z;

	m_values[Jx] = jBasis3D.x;
	m_values[Jy] = jBasis3D.y;
	m_values[Jz] = jBasis3D.z;

	m_values[Kx] = kBasis3D.x;
	m_values[Ky] = kBasis3D.y;
	m_values[Kz] = kBasis3D.z;

	m_values[Tx] = translation3D.x;
	m_values[Ty] = translation3D.y;
	m_values[Tz] = translation3D.z;
}

Mat44::Mat44(Vec4 const& iBasis4D, Vec4 const& jBasis4D, Vec4 const& kBasis4D, Vec4 const& translation4D)
{
	m_values[Ix] = iBasis4D.x;
	m_values[Iy] = iBasis4D.y;
	m_values[Iz] = iBasis4D.z;
	m_values[Iw] = iBasis4D.w;

	m_values[Jx] = jBasis4D.x;
	m_values[Jy] = jBasis4D.y;
	m_values[Jz] = jBasis4D.z;
	m_values[Jw] = jBasis4D.w;

	m_values[Kx] = kBasis4D.x;
	m_values[Ky] = kBasis4D.y;
	m_values[Kz] = kBasis4D.z;
	m_values[Kw] = kBasis4D.w;

	m_values[Tx] = translation4D.x;
	m_values[Ty] = translation4D.y;
	m_values[Tz] = translation4D.z;
	m_values[Tw] = translation4D.w;
}

Mat44::Mat44(float const* sixteenValuesBasisMajor)
{
	m_values[Ix] = sixteenValuesBasisMajor[0];
	m_values[Iy] = sixteenValuesBasisMajor[1];
	m_values[Iz] = sixteenValuesBasisMajor[2];
	m_values[Iw] = sixteenValuesBasisMajor[3];

	m_values[Jx] = sixteenValuesBasisMajor[4];
	m_values[Jy] = sixteenValuesBasisMajor[5];
	m_values[Jz] = sixteenValuesBasisMajor[6];
	m_values[Jw] = sixteenValuesBasisMajor[7];
				   
	m_values[Kx] = sixteenValuesBasisMajor[8];
	m_values[Ky] = sixteenValuesBasisMajor[9];
	m_values[Kz] = sixteenValuesBasisMajor[10];
	m_values[Kw] = sixteenValuesBasisMajor[11];
				  
	m_values[Tx] = sixteenValuesBasisMajor[12];
	m_values[Ty] = sixteenValuesBasisMajor[13];
	m_values[Tz] = sixteenValuesBasisMajor[14];
	m_values[Tw] = sixteenValuesBasisMajor[15];
}

Mat44::Mat44(Vec3 const& position, EulerAngles const& orientation, Vec3 const& scale)
{
	SetTranslation3D(position);
	Mat44 rotation = orientation.GetAsMatrix_IFwd_JLeft_KUp();
	Append(rotation);
	AppendScaleNonUniform3D(scale);
}

Mat44 const Mat44::MakeTranslation2D(Vec2 const& translationXY)
{
	Mat44 newMatrix;
	newMatrix.m_values[Tx] = translationXY.x;
	newMatrix.m_values[Ty] = translationXY.y;
	return newMatrix;
}

Mat44 const Mat44::MakeTranslation3D(Vec3 const& translationXYZ)
{
	Mat44 newMatrix;
	newMatrix.m_values[Tx] = translationXYZ.x;
	newMatrix.m_values[Ty] = translationXYZ.y;
	newMatrix.m_values[Tz] = translationXYZ.z;
	return newMatrix;
}

Mat44 const Mat44::MakeUniformScale2D(float uniformScaleXY)
{
	Mat44 newMatrix;
	newMatrix.m_values[Ix] = uniformScaleXY;
	newMatrix.m_values[Jy] = uniformScaleXY;
	return newMatrix;
}

Mat44 const Mat44::MakeUniformScale3D(float unfiformScaleXYZ)
{
	Mat44 newMatrix;
	newMatrix.m_values[Ix] = unfiformScaleXYZ;
	newMatrix.m_values[Jy] = unfiformScaleXYZ;
	newMatrix.m_values[Kz] = unfiformScaleXYZ;
	return newMatrix;
}

Mat44 const Mat44::MakeNonUniformScale2D(Vec2 const& nonUniformScaleXY)
{
	Mat44 newMatrix;
	newMatrix.m_values[Ix] = nonUniformScaleXY.x;
	newMatrix.m_values[Jy] = nonUniformScaleXY.y;
	return newMatrix;
}

Mat44 const Mat44::MakeNonUniformScale3D(Vec3 const& nonUniformScaleXYZ)
{
	Mat44 newMatrix;
	newMatrix.m_values[Ix] = nonUniformScaleXYZ.x;
	newMatrix.m_values[Jy] = nonUniformScaleXYZ.y;
	newMatrix.m_values[Kz] = nonUniformScaleXYZ.z;
	return newMatrix;
}

Mat44 const Mat44::MakeZRotationDegrees(float rotationDegreesAboutZ)
{
	Mat44 newMatrix;
	newMatrix.m_values[Ix] = CosDegrees(rotationDegreesAboutZ);
	newMatrix.m_values[Iy] = SinDegrees(rotationDegreesAboutZ);
	newMatrix.m_values[Jx] = -SinDegrees(rotationDegreesAboutZ);
	newMatrix.m_values[Jy] = CosDegrees(rotationDegreesAboutZ);
	return newMatrix;
}

Mat44 const Mat44::MakeYRotationDegrees(float rotationDegreesAboutY)
{
	Mat44 newMatrix;
	newMatrix.m_values[Ix] = CosDegrees(rotationDegreesAboutY);
	newMatrix.m_values[Iz] = -SinDegrees(rotationDegreesAboutY);
	newMatrix.m_values[Kx] = SinDegrees(rotationDegreesAboutY);
	newMatrix.m_values[Kz] = CosDegrees(rotationDegreesAboutY);
	return newMatrix;
}

Mat44 const Mat44::MakeXRotationDegrees(float rotationDegreesAboutX)
{
	Mat44 newMatrix;
	newMatrix.m_values[Jy] = CosDegrees(rotationDegreesAboutX);
	newMatrix.m_values[Jz] = SinDegrees(rotationDegreesAboutX);
	newMatrix.m_values[Ky] = -SinDegrees(rotationDegreesAboutX);
	newMatrix.m_values[Kz] = CosDegrees(rotationDegreesAboutX);
	return newMatrix;
}

Mat44 const Mat44::MakeOrthoProjection(float left, float right, float bottom, float top, float zNear, float zFar)
{
	Mat44 projectedMat;
	projectedMat.m_values[Ix] = 2.f / (right - left);
	projectedMat.m_values[Tx] = (left + right) / (left - right);

	projectedMat.m_values[Jy] = 2.f / (top - bottom);
	projectedMat.m_values[Ty] = (bottom + top) / (bottom - top);

	projectedMat.m_values[Kz] = 1.f / (zFar - zNear);
	projectedMat.m_values[Tz] = zNear  / (zNear - zFar);

	return projectedMat;
}

Mat44 const Mat44::MakePerspectiveProjection(float fovYDegrees, float aspect, float zNear, float zFar)
{
	Mat44 perspective;
	
	float c = CosDegrees(fovYDegrees * 0.5f);
	float s = SinDegrees(fovYDegrees * 0.5f);
	float scaleY = c / s;
	float scaleX = scaleY / aspect;
	float scaleZ = zFar / (zFar - zNear);
	float translateZ = (zNear * zFar) / (zNear - zFar);

	perspective.m_values[Ix] = scaleX;
	perspective.m_values[Jy] = scaleY;
	perspective.m_values[Kz] = scaleZ;
	perspective.m_values[Kw] = 1.f;
	perspective.m_values[Tz] = translateZ;
	perspective.m_values[Tw] = 0.f;
	return perspective;
}

Vec2 const Mat44::TransformVectorQuantity2D(Vec2 const& vectorQuantityXY) const
{
	Vec2 vectorQuantity2D;
	vectorQuantity2D.x = (m_values[Ix] * vectorQuantityXY.x) + (m_values[Jx] * vectorQuantityXY.y);
	vectorQuantity2D.y = (m_values[Iy] * vectorQuantityXY.x) + (m_values[Jy] * vectorQuantityXY.y);
	return vectorQuantity2D;
}

Vec3 const Mat44::TransformVectorQuantity3D(Vec3 const& vectorQuantityXYZ) const
{
	Vec3 vectorQuantity3D;
	vectorQuantity3D.x = (m_values[Ix] * vectorQuantityXYZ.x) + (m_values[Jx] * vectorQuantityXYZ.y) + (m_values[Kx] * vectorQuantityXYZ.z);
	vectorQuantity3D.y = (m_values[Iy] * vectorQuantityXYZ.x) + (m_values[Jy] * vectorQuantityXYZ.y) + (m_values[Ky] * vectorQuantityXYZ.z);
	vectorQuantity3D.z = (m_values[Iz] * vectorQuantityXYZ.x) + (m_values[Jz] * vectorQuantityXYZ.y) + (m_values[Kz] * vectorQuantityXYZ.z);
	return vectorQuantity3D;
}

Vec2 const Mat44::TransformPosition2D(Vec2 const& positionXY) const
{
	Vec2 position2D;
	position2D.x = (m_values[Ix] * positionXY.x) + (m_values[Jx] * positionXY.y) + m_values[Tx];
	position2D.y = (m_values[Iy] * positionXY.x) + (m_values[Jy] * positionXY.y) + m_values[Ty];
	return position2D;
}

Vec3 const Mat44::TransformPosition3D(Vec3 const& positionXYZ) const
{
	Vec3 position3D;
	position3D.x = (m_values[Ix] * positionXYZ.x) + (m_values[Jx] * positionXYZ.y) + (m_values[Kx] * positionXYZ.z) + m_values[Tx];
	position3D.y = (m_values[Iy] * positionXYZ.x) + (m_values[Jy] * positionXYZ.y) + (m_values[Ky] * positionXYZ.z) + m_values[Ty];
	position3D.z = (m_values[Iz] * positionXYZ.x) + (m_values[Jz] * positionXYZ.y) + (m_values[Kz] * positionXYZ.z) + m_values[Tz];
	return position3D;
}

Vec4 const Mat44::TransformHomogeneous3D(Vec4 const& homogenousPoint3D) const
{
	Vec4 point4D;
	point4D.x = (m_values[Ix] * homogenousPoint3D.x) + (m_values[Jx] * homogenousPoint3D.y) + (m_values[Kx] * homogenousPoint3D.z) + (m_values[Tx] * homogenousPoint3D.w);
	point4D.y = (m_values[Iy] * homogenousPoint3D.x) + (m_values[Jy] * homogenousPoint3D.y) + (m_values[Ky] * homogenousPoint3D.z) + (m_values[Ty] * homogenousPoint3D.w);
	point4D.z = (m_values[Iz] * homogenousPoint3D.x) + (m_values[Jz] * homogenousPoint3D.y) + (m_values[Kz] * homogenousPoint3D.z) + (m_values[Tz] * homogenousPoint3D.w);
	point4D.w = (m_values[Iw] * homogenousPoint3D.x) + (m_values[Jw] * homogenousPoint3D.y) + (m_values[Kw] * homogenousPoint3D.z) + (m_values[Tw] * homogenousPoint3D.w);
	return point4D;
}


float* Mat44::GetAsFloatArray()
{
	return m_values;
}

float const* Mat44::GetAsFloatArray() const
{
	return m_values;
}

Vec2 const Mat44::GetIBasis2D() const
{
	return Vec2(m_values[Ix], m_values[Iy]);
}

Vec2 const Mat44::GetJBasis2D() const
{
	return Vec2(m_values[Jx], m_values[Jy]);
}

Vec2 const Mat44::GetTranslation2D() const
{
	return Vec2(m_values[Tx], m_values[Ty]);
}

Vec3 const Mat44::GetIBasis3D() const
{
	return Vec3(m_values[Ix], m_values[Iy], m_values[Iz]);
}

Vec3 const Mat44::GetJBasis3D() const
{
	return Vec3(m_values[Jx], m_values[Jy], m_values[Jz]);
}

Vec3 const Mat44::GetKBasis3D() const
{
	return Vec3(m_values[Kx], m_values[Ky], m_values[Kz]);
}

Vec3 const Mat44::GetTranslation3D() const
{
	return Vec3(m_values[Tx], m_values[Ty], m_values[Tz]);
}

Vec4 const Mat44::GetIBasis4D() const
{
	return Vec4(m_values[Ix], m_values[Iy], m_values[Iz], m_values[Iw]);
}

Vec4 const Mat44::GetJBasis4D() const
{
	return Vec4(m_values[Jx], m_values[Jy], m_values[Jz], m_values[Jw]);
}

Vec4 const Mat44::GetKBasis4D() const
{
	return Vec4(m_values[Kx], m_values[Ky], m_values[Kz], m_values[Kw]);
}

Vec4 const Mat44::GetTranslation4D() const
{
	return Vec4(m_values[Tx], m_values[Ty], m_values[Tz], m_values[Tw]);
}

Mat44 const Mat44::GetOrthonormalInverse() const
{
	Mat44 inverseMatrix;

	Vec3 iBasis(m_values[Ix], m_values[Jx], m_values[Kx]);
	Vec3 jBasis(m_values[Iy], m_values[Jy], m_values[Ky]);
	Vec3 kBasis(m_values[Iz], m_values[Jz], m_values[Kz]);

	float tx = (-m_values[Ix] * m_values[Tx]) - (m_values[Iy] * m_values[Ty]) - (m_values[Iz] * m_values[Tz]);
	float ty = (-m_values[Jx] * m_values[Tx]) - (m_values[Jy] * m_values[Ty]) - (m_values[Jz] * m_values[Tz]);
	float tz = (-m_values[Kx] * m_values[Tx]) - (m_values[Ky] * m_values[Ty]) - (m_values[Kz] * m_values[Tz]);
	Vec3 translation(tx, ty, tz);

	return Mat44(iBasis, jBasis, kBasis, translation);
}

Mat44 const Mat44::GetInverse() const
{
	float augmented[4][8] = {};

	for (int rowIndex = 0; rowIndex < 4; ++rowIndex)
	{
		for (int columnIndex = 0; columnIndex < 4; ++columnIndex)
		{
			int sourceIndex = ((columnIndex * 4) + rowIndex);
			augmented[rowIndex][columnIndex] = m_values[sourceIndex];
		}

		for (int identityColumnIndex = 0; identityColumnIndex < 4; ++identityColumnIndex)
		{
			augmented[rowIndex][identityColumnIndex + 4] = (rowIndex == identityColumnIndex) ? 1.0f : 0.0f;
		}
	}

	for (int pivotColumnIndex = 0; pivotColumnIndex < 4; ++pivotColumnIndex)
	{
		int bestPivotRowIndex = pivotColumnIndex;
		float bestPivotMagnitude = fabsf(augmented[pivotColumnIndex][pivotColumnIndex]);

		for (int candidateRowIndex = (pivotColumnIndex + 1); candidateRowIndex < 4; ++candidateRowIndex)
		{
			float candidateMagnitude = fabsf(augmented[candidateRowIndex][pivotColumnIndex]);
			if (candidateMagnitude > bestPivotMagnitude)
			{
				bestPivotMagnitude = candidateMagnitude;
				bestPivotRowIndex = candidateRowIndex;
			}
		}

		if (bestPivotMagnitude <= 0.000001f)
		{
			ERROR_AND_DIE("Mat44::GetInverse failed because the matrix is singular.");
		}

		if (bestPivotRowIndex != pivotColumnIndex)
		{
			for (int columnIndex = 0; columnIndex < 8; ++columnIndex)
			{
				float tempValue = augmented[pivotColumnIndex][columnIndex];
				augmented[pivotColumnIndex][columnIndex] = augmented[bestPivotRowIndex][columnIndex];
				augmented[bestPivotRowIndex][columnIndex] = tempValue;
			}
		}

		float pivotValue = augmented[pivotColumnIndex][pivotColumnIndex];
		float inversePivotValue = (1.0f / pivotValue);

		for (int columnIndex = 0; columnIndex < 8; ++columnIndex)
		{
			augmented[pivotColumnIndex][columnIndex] *= inversePivotValue;
		}

		for (int rowIndex = 0; rowIndex < 4; ++rowIndex)
		{
			if (rowIndex == pivotColumnIndex)
			{
				continue;
			}

			float eliminationFactor = augmented[rowIndex][pivotColumnIndex];
			if (eliminationFactor == 0.0f)
			{
				continue;
			}

			for (int columnIndex = 0; columnIndex < 8; ++columnIndex)
			{
				augmented[rowIndex][columnIndex] -= (eliminationFactor * augmented[pivotColumnIndex][columnIndex]);
			}
		}
	}

	Mat44 inverseMatrix;

	for (int rowIndex = 0; rowIndex < 4; ++rowIndex)
	{
		for (int columnIndex = 0; columnIndex < 4; ++columnIndex)
		{
			int destinationIndex = ((columnIndex * 4) + rowIndex);
			inverseMatrix.m_values[destinationIndex] = augmented[rowIndex][columnIndex + 4];
		}
	}

	return inverseMatrix;
}

Mat44 const Mat44::GetTransposed3x3() const
{
	Mat44 result(m_values);

	result.m_values[Ix] = m_values[Ix];
	result.m_values[Iy] = m_values[Jx];
	result.m_values[Iz] = m_values[Kx];

	result.m_values[Jx] = m_values[Iy];
	result.m_values[Jy] = m_values[Jy];
	result.m_values[Jz] = m_values[Ky];

	result.m_values[Kx] = m_values[Iz];
	result.m_values[Ky] = m_values[Jz];
	result.m_values[Kz] = m_values[Kz];

	return result;
};

EulerAngles const Mat44::GetOrientation() const
{
	Vec3 i = GetIBasis3D(); // forward (x)
	Vec3 j = GetJBasis3D(); // left (y)
	Vec3 k = GetKBasis3D(); // up (z)

	// Yaw: rotation around Z (turning left/right)
	float yaw = atan2f(i.y, i.x);

	// Pitch: rotation around Y (looking up/down)
	float forwardLength = sqrtf(i.x * i.x + i.y * i.y);
	float pitch = -atan2f(i.z, forwardLength);

	// Roll: rotation around X (tilt)
	float roll = atan2f(j.z, k.z);

	EulerAngles result;
	result.m_yawDegrees = ConvertRadiansToDegrees(yaw);
	result.m_pitchDegrees = ConvertRadiansToDegrees(pitch);
	result.m_rollDegrees = ConvertRadiansToDegrees(roll);
	return result;
}

void Mat44::SetTranslation2D(Vec2 const& translationXY)
{
	m_values[Tx] = translationXY.x;
	m_values[Ty] = translationXY.y;
	m_values[Tz] = 0.f;
	m_values[Tw] = 1.f;
}

void Mat44::SetTranslation3D(Vec3 const& translationXYZ)
{
	m_values[Tx] = translationXYZ.x;
	m_values[Ty] = translationXYZ.y;
	m_values[Tz] = translationXYZ.z;
	m_values[Tw] = 1.f;
}

void Mat44::SetIJ2D(Vec2 const& iBasis2D, Vec2 const& jBasis2D)
{
	m_values[Ix] = iBasis2D.x;
	m_values[Iy] = iBasis2D.y;
	m_values[Iz] = 0.f;
	m_values[Iw] = 0.f;

	m_values[Jx] = jBasis2D.x;
	m_values[Jy] = jBasis2D.y;
	m_values[Jz] = 0.f;
	m_values[Jw] = 0.f;
}

void Mat44::SetIJT2D(Vec2 const& iBasis2D, Vec2 const& jBasis2D, Vec2 const& translation2D)
{
	m_values[Ix] = iBasis2D.x;
	m_values[Iy] = iBasis2D.y;
	m_values[Iz] = 0.f;
	m_values[Iw] = 0.f;

	m_values[Jx] = jBasis2D.x;
	m_values[Jy] = jBasis2D.y;
	m_values[Jz] = 0.f;
	m_values[Jw] = 0.f;

	m_values[Tx] = translation2D.x;
	m_values[Ty] = translation2D.y;
	m_values[Tz] = 0.f;
	m_values[Tw] = 1.f;
}

void Mat44::SetIJK3D(Vec3 const& iBasis3D, Vec3 const& jBasis3D, Vec3 const& kBasis3D)
{
	m_values[Ix] = iBasis3D.x;
	m_values[Iy] = iBasis3D.y;
	m_values[Iz] = iBasis3D.z;
	m_values[Iw] = 0.f;

	m_values[Jx] = jBasis3D.x;
	m_values[Jy] = jBasis3D.y;
	m_values[Jz] = jBasis3D.z;
	m_values[Jw] = 0.f;

	m_values[Kx] = kBasis3D.x;
	m_values[Ky] = kBasis3D.y;
	m_values[Kz] = kBasis3D.z;
	m_values[Kw] = 0.f;
}

void Mat44::SetIJKT3D(Vec3 const& iBasis3D, Vec3 const& jBasis3D, Vec3 const& kBasis3D, Vec3 const& translationXYZ)
{
	m_values[Ix] = iBasis3D.x;
	m_values[Iy] = iBasis3D.y;
	m_values[Iz] = iBasis3D.z;
	m_values[Iw] = 0.f;

	m_values[Jx] = jBasis3D.x;
	m_values[Jy] = jBasis3D.y;
	m_values[Jz] = jBasis3D.z;
	m_values[Jw] = 0.f;

	m_values[Kx] = kBasis3D.x;
	m_values[Ky] = kBasis3D.y;
	m_values[Kz] = kBasis3D.z;
	m_values[Kw] = 0.f;

	m_values[Tx] = translationXYZ.x;
	m_values[Ty] = translationXYZ.y;
	m_values[Tz] = translationXYZ.z;
	m_values[Tw] = 1.f;
}

void Mat44::SetIJKT4D(Vec4 const& iBasis4D, Vec4 const& jBasis4D, Vec4 const& kBasis4D, Vec4 const& translation4D)
{
	m_values[Ix] = iBasis4D.x;
	m_values[Iy] = iBasis4D.y;
	m_values[Iz] = iBasis4D.z;
	m_values[Iw] = iBasis4D.w;

	m_values[Jx] = jBasis4D.x;
	m_values[Jy] = jBasis4D.y;
	m_values[Jz] = jBasis4D.z;
	m_values[Jw] = jBasis4D.w;

	m_values[Kx] = kBasis4D.x;
	m_values[Ky] = kBasis4D.y;
	m_values[Kz] = kBasis4D.z;
	m_values[Kw] = kBasis4D.w;

	m_values[Tx] = translation4D.x;
	m_values[Ty] = translation4D.y;
	m_values[Tz] = translation4D.z;
	m_values[Tw] = translation4D.w;
}

void Mat44::Transpose()
{
	Mat44 oldMatt(m_values);
	m_values[Jx] = oldMatt.m_values[Iy];
	m_values[Iy] = oldMatt.m_values[Jx];
	m_values[Kx] = oldMatt.m_values[Iz];
	m_values[Iz] = oldMatt.m_values[Kx];
	m_values[Tx] = oldMatt.m_values[Iw];
	m_values[Iw] = oldMatt.m_values[Tx];
	m_values[Ky] = oldMatt.m_values[Jz];
	m_values[Jz] = oldMatt.m_values[Ky];
	m_values[Ty] = oldMatt.m_values[Jw];
	m_values[Jw] = oldMatt.m_values[Ty];
	m_values[Tz] = oldMatt.m_values[Kw];
	m_values[Kw] = oldMatt.m_values[Tz];
}


void Mat44::Orthonormalize_IFwd_JLeft_KUp()
{
	Vec3 iBasis = GetIBasis3D();
	Vec3 kBasis = GetKBasis3D();
	Vec3 jBasis = CrossProduct3D(kBasis, iBasis);
	//Add check and reversal here
	kBasis =  CrossProduct3D(iBasis, jBasis);
	iBasis.Normalize();
	jBasis.Normalize();
	kBasis.Normalize();
	SetIJK3D(iBasis, jBasis, kBasis);
}

void Mat44::Orthonormalize_IFwd_JRight_KUp()
{
	Vec3 iBasis = GetIBasis3D();
	Vec3 kBasis = GetKBasis3D();
	Vec3 jBasis = CrossProduct3D(iBasis, kBasis);
	kBasis = CrossProduct3D(jBasis, iBasis);
	iBasis.Normalize();
	jBasis.Normalize();
	kBasis.Normalize();
	SetIJK3D(iBasis, jBasis, kBasis);
}

void Mat44::Append(Mat44 const& appendThis)
{
	Mat44 oldMatrix(m_values);
	m_values[Ix] = (oldMatrix.m_values[Ix] * appendThis.m_values[Ix]) + (oldMatrix.m_values[Jx] * appendThis.m_values[Iy])
					+ (oldMatrix.m_values[Kx] * appendThis.m_values[Iz]) + (oldMatrix.m_values[Tx] * appendThis.m_values[Iw]);

	m_values[Iy] = (oldMatrix.m_values[Iy] * appendThis.m_values[Ix]) + (oldMatrix.m_values[Jy] * appendThis.m_values[Iy])
					+ (oldMatrix.m_values[Ky] * appendThis.m_values[Iz]) + (oldMatrix.m_values[Ty] * appendThis.m_values[Iw]);

	m_values[Iz] = (oldMatrix.m_values[Iz] * appendThis.m_values[Ix]) + (oldMatrix.m_values[Jz] * appendThis.m_values[Iy])
					+ (oldMatrix.m_values[Kz] * appendThis.m_values[Iz]) + (oldMatrix.m_values[Tz] * appendThis.m_values[Iw]);

	m_values[Iw] = (oldMatrix.m_values[Iw] * appendThis.m_values[Ix]) + (oldMatrix.m_values[Jw] * appendThis.m_values[Iy])
					+ (oldMatrix.m_values[Kw] * appendThis.m_values[Iz]) + (oldMatrix.m_values[Tw] * appendThis.m_values[Iw]);



	m_values[Jx] = (oldMatrix.m_values[Ix] * appendThis.m_values[Jx]) + (oldMatrix.m_values[Jx] * appendThis.m_values[Jy])
					+ (oldMatrix.m_values[Kx] * appendThis.m_values[Jz]) + (oldMatrix.m_values[Tx] * appendThis.m_values[Jw]);

	m_values[Jy] = (oldMatrix.m_values[Iy] * appendThis.m_values[Jx]) + (oldMatrix.m_values[Jy] * appendThis.m_values[Jy])
					+ (oldMatrix.m_values[Ky] * appendThis.m_values[Jz]) + (oldMatrix.m_values[Ty] * appendThis.m_values[Jw]);

	m_values[Jz] = (oldMatrix.m_values[Iz] * appendThis.m_values[Jx]) + (oldMatrix.m_values[Jz] * appendThis.m_values[Jy]) 
					+ (oldMatrix.m_values[Kz] * appendThis.m_values[Jz]) + (oldMatrix.m_values[Tz] * appendThis.m_values[Jw]);

	m_values[Jw] = (oldMatrix.m_values[Iw] * appendThis.m_values[Jx]) + (oldMatrix.m_values[Jw] * appendThis.m_values[Jy])
					+ (oldMatrix.m_values[Kw] * appendThis.m_values[Jz]) + (oldMatrix.m_values[Tw] * appendThis.m_values[Jw]);



	m_values[Kx] = (oldMatrix.m_values[Ix] * appendThis.m_values[Kx]) + (oldMatrix.m_values[Jx] * appendThis.m_values[Ky])
					+ (oldMatrix.m_values[Kx] * appendThis.m_values[Kz]) + (oldMatrix.m_values[Tx] * appendThis.m_values[Kw]);

	m_values[Ky] = (oldMatrix.m_values[Iy] * appendThis.m_values[Kx]) + (oldMatrix.m_values[Jy] * appendThis.m_values[Ky])
					+ (oldMatrix.m_values[Ky] * appendThis.m_values[Kz]) + (oldMatrix.m_values[Ty] * appendThis.m_values[Kw]);

	m_values[Kz] = (oldMatrix.m_values[Iz] * appendThis.m_values[Kx]) + (oldMatrix.m_values[Jz] * appendThis.m_values[Ky])
					+ (oldMatrix.m_values[Kz] * appendThis.m_values[Kz]) + (oldMatrix.m_values[Tz] * appendThis.m_values[Kw]);

	m_values[Kw] = (oldMatrix.m_values[Iw] * appendThis.m_values[Kx]) + (oldMatrix.m_values[Jw] * appendThis.m_values[Ky])
					+ (oldMatrix.m_values[Kw] * appendThis.m_values[Kz]) + (oldMatrix.m_values[Tw] * appendThis.m_values[Kw]);


	m_values[Tx] = (oldMatrix.m_values[Ix] * appendThis.m_values[Tx]) + (oldMatrix.m_values[Jx] * appendThis.m_values[Ty])
					+ (oldMatrix.m_values[Kx] * appendThis.m_values[Tz]) + (oldMatrix.m_values[Tx] * appendThis.m_values[Tw]);

	m_values[Ty] = (oldMatrix.m_values[Iy] * appendThis.m_values[Tx]) + (oldMatrix.m_values[Jy] * appendThis.m_values[Ty])
					+ (oldMatrix.m_values[Ky] * appendThis.m_values[Tz]) + (oldMatrix.m_values[Ty] * appendThis.m_values[Tw]);

	m_values[Tz] = (oldMatrix.m_values[Iz] * appendThis.m_values[Tx]) + (oldMatrix.m_values[Jz] * appendThis.m_values[Ty])
					+ (oldMatrix.m_values[Kz] * appendThis.m_values[Tz]) + (oldMatrix.m_values[Tz] * appendThis.m_values[Tw]);

	m_values[Tw] = (oldMatrix.m_values[Iw] * appendThis.m_values[Tx]) + (oldMatrix.m_values[Jw] * appendThis.m_values[Ty])
					+ (oldMatrix.m_values[Kw] * appendThis.m_values[Tz]) + (oldMatrix.m_values[Tw] * appendThis.m_values[Tw]);

}

void Mat44::AppendZRotation(float degreesRotationAboutZ)
{
	Mat44 appendThis = Mat44::MakeZRotationDegrees(degreesRotationAboutZ);
	Mat44 oldMatrix(m_values);

	m_values[Ix] = (oldMatrix.m_values[Ix] * appendThis.m_values[Ix]) + (oldMatrix.m_values[Jx] * appendThis.m_values[Iy]);
	m_values[Iy] = (oldMatrix.m_values[Iy] * appendThis.m_values[Ix]) + (oldMatrix.m_values[Jy] * appendThis.m_values[Iy]);
	m_values[Iz] = (oldMatrix.m_values[Iz] * appendThis.m_values[Ix]) + (oldMatrix.m_values[Jz] * appendThis.m_values[Iy]);
	m_values[Iw] = (oldMatrix.m_values[Iw] * appendThis.m_values[Ix]) + (oldMatrix.m_values[Jw] * appendThis.m_values[Iy]);

	m_values[Jx] = (oldMatrix.m_values[Ix] * appendThis.m_values[Jx]) + (oldMatrix.m_values[Jx] * appendThis.m_values[Jy]);
	m_values[Jy] = (oldMatrix.m_values[Iy] * appendThis.m_values[Jx]) + (oldMatrix.m_values[Jy] * appendThis.m_values[Jy]);
	m_values[Jz] = (oldMatrix.m_values[Iz] * appendThis.m_values[Jx]) + (oldMatrix.m_values[Jz] * appendThis.m_values[Jy]);
	m_values[Jw] = (oldMatrix.m_values[Iw] * appendThis.m_values[Jx]) + (oldMatrix.m_values[Jw] * appendThis.m_values[Jy]);
}

void Mat44::AppendYRotation(float degreesRotationAboutY)
{
	Mat44 appendThis = Mat44::MakeYRotationDegrees(degreesRotationAboutY);
	Mat44 oldMatrix(m_values);

	m_values[Ix] = (oldMatrix.m_values[Ix] * appendThis.m_values[Ix]) + (oldMatrix.m_values[Kx] * appendThis.m_values[Iz]);
	m_values[Iy] = (oldMatrix.m_values[Iy] * appendThis.m_values[Ix]) + (oldMatrix.m_values[Ky] * appendThis.m_values[Iz]);
	m_values[Iz] = (oldMatrix.m_values[Iz] * appendThis.m_values[Ix]) + (oldMatrix.m_values[Kz] * appendThis.m_values[Iz]);
	m_values[Iw] = (oldMatrix.m_values[Iw] * appendThis.m_values[Ix]) + (oldMatrix.m_values[Kw] * appendThis.m_values[Iz]);

	m_values[Kx] = (oldMatrix.m_values[Ix] * appendThis.m_values[Kx]) + (oldMatrix.m_values[Kx] * appendThis.m_values[Kz]);
	m_values[Ky] = (oldMatrix.m_values[Iy] * appendThis.m_values[Kx]) + (oldMatrix.m_values[Ky] * appendThis.m_values[Kz]);
	m_values[Kz] = (oldMatrix.m_values[Iz] * appendThis.m_values[Kx]) + (oldMatrix.m_values[Kz] * appendThis.m_values[Kz]);
	m_values[Kw] = (oldMatrix.m_values[Iw] * appendThis.m_values[Kx]) + (oldMatrix.m_values[Kw] * appendThis.m_values[Kz]);
}

void Mat44::AppendXRotation(float degreesRotationAboutX)
{
	Mat44 appendThis = Mat44::MakeXRotationDegrees(degreesRotationAboutX);
	Mat44 oldMatrix(m_values);

	m_values[Jx] = (oldMatrix.m_values[Jx] * appendThis.m_values[Jy]) + (oldMatrix.m_values[Kx] * appendThis.m_values[Jz]);
	m_values[Jy] = (oldMatrix.m_values[Jy] * appendThis.m_values[Jy]) + (oldMatrix.m_values[Ky] * appendThis.m_values[Jz]);
	m_values[Jz] = (oldMatrix.m_values[Jz] * appendThis.m_values[Jy]) + (oldMatrix.m_values[Kz] * appendThis.m_values[Jz]);
	m_values[Jw] = (oldMatrix.m_values[Jw] * appendThis.m_values[Jy]) + (oldMatrix.m_values[Kw] * appendThis.m_values[Jz]);

	m_values[Kx] = (oldMatrix.m_values[Jx] * appendThis.m_values[Ky]) + (oldMatrix.m_values[Kx] * appendThis.m_values[Kz]);
	m_values[Ky] = (oldMatrix.m_values[Jy] * appendThis.m_values[Ky]) + (oldMatrix.m_values[Ky] * appendThis.m_values[Kz]);
	m_values[Kz] = (oldMatrix.m_values[Jz] * appendThis.m_values[Ky]) + (oldMatrix.m_values[Kz] * appendThis.m_values[Kz]);
	m_values[Kw] = (oldMatrix.m_values[Jw] * appendThis.m_values[Ky]) + (oldMatrix.m_values[Kw] * appendThis.m_values[Kz]);
}

void Mat44::AppendTranslation2D(Vec2 const& translationXY)
{
	Mat44 appendThis;
	appendThis.SetTranslation2D(translationXY);
	Mat44 oldMatrix(m_values);

	m_values[Tx] = (oldMatrix.m_values[Ix] * appendThis.m_values[Tx]) + (oldMatrix.m_values[Jx] * appendThis.m_values[Ty])
		+ (oldMatrix.m_values[Tx] * appendThis.m_values[Tw]);

	m_values[Ty] = (oldMatrix.m_values[Iy] * appendThis.m_values[Tx]) + (oldMatrix.m_values[Jy] * appendThis.m_values[Ty])
		+ (oldMatrix.m_values[Ty] * appendThis.m_values[Tw]);

	m_values[Tz] = (oldMatrix.m_values[Iz] * appendThis.m_values[Tx]) + (oldMatrix.m_values[Jz] * appendThis.m_values[Ty])
		+ (oldMatrix.m_values[Tz] * appendThis.m_values[Tw]);

	m_values[Tw] = (oldMatrix.m_values[Iw] * appendThis.m_values[Tx]) + (oldMatrix.m_values[Jw] * appendThis.m_values[Ty])
		+(oldMatrix.m_values[Tw] * appendThis.m_values[Tw]);
}

void Mat44::AppendTranslation3D(Vec3 const& translationXYZ)
{
	Mat44 appendThis;
	appendThis.SetTranslation3D(translationXYZ);
	Mat44 oldMatrix(m_values);

	m_values[Tx] = (oldMatrix.m_values[Ix] * appendThis.m_values[Tx]) + (oldMatrix.m_values[Jx] * appendThis.m_values[Ty])
		+ (oldMatrix.m_values[Kx] * appendThis.m_values[Tz]) + (oldMatrix.m_values[Tx] * appendThis.m_values[Tw]);

	m_values[Ty] = (oldMatrix.m_values[Iy] * appendThis.m_values[Tx]) + (oldMatrix.m_values[Jy] * appendThis.m_values[Ty])
		+ (oldMatrix.m_values[Ky] * appendThis.m_values[Tz]) + (oldMatrix.m_values[Ty] * appendThis.m_values[Tw]);

	m_values[Tz] = (oldMatrix.m_values[Iz] * appendThis.m_values[Tx]) + (oldMatrix.m_values[Jz] * appendThis.m_values[Ty])
		+ (oldMatrix.m_values[Kz] * appendThis.m_values[Tz]) + (oldMatrix.m_values[Tz] * appendThis.m_values[Tw]);

	m_values[Tw] = (oldMatrix.m_values[Iw] * appendThis.m_values[Tx]) + (oldMatrix.m_values[Jw] * appendThis.m_values[Ty])
		+ (oldMatrix.m_values[Kw] * appendThis.m_values[Tz]) + (oldMatrix.m_values[Tw] * appendThis.m_values[Tw]);
}

void Mat44::AppendScaleUniform2D(float uniformScaleXY)
{
	Mat44 oldMatrix(m_values);
						   
	m_values[Ix] = (oldMatrix.m_values[Ix] * uniformScaleXY);
	m_values[Iy] = (oldMatrix.m_values[Iy] * uniformScaleXY);
	m_values[Iz] = (oldMatrix.m_values[Iz] * uniformScaleXY);
	m_values[Iw] = (oldMatrix.m_values[Iw] * uniformScaleXY);

	m_values[Jx] = (oldMatrix.m_values[Jx] * uniformScaleXY);
	m_values[Jy] = (oldMatrix.m_values[Jy] * uniformScaleXY);
	m_values[Jz] = (oldMatrix.m_values[Jz] * uniformScaleXY);
	m_values[Jw] = (oldMatrix.m_values[Jw] * uniformScaleXY);
}

void Mat44::AppendScaleUniform3D(float uniformScaleXYZ)
{
	Mat44 oldMatrix(m_values);

	m_values[Ix] = (oldMatrix.m_values[Ix] * uniformScaleXYZ);
	m_values[Iy] = (oldMatrix.m_values[Iy] * uniformScaleXYZ);
	m_values[Iz] = (oldMatrix.m_values[Iz] * uniformScaleXYZ);
	m_values[Iw] = (oldMatrix.m_values[Iw] * uniformScaleXYZ);
											 
	m_values[Jx] = (oldMatrix.m_values[Jx] * uniformScaleXYZ);
	m_values[Jy] = (oldMatrix.m_values[Jy] * uniformScaleXYZ);
	m_values[Jz] = (oldMatrix.m_values[Jz] * uniformScaleXYZ);
	m_values[Jw] = (oldMatrix.m_values[Jw] * uniformScaleXYZ);
											 
	m_values[Kx] = (oldMatrix.m_values[Kx] * uniformScaleXYZ);
	m_values[Ky] = (oldMatrix.m_values[Ky] * uniformScaleXYZ);
	m_values[Kz] = (oldMatrix.m_values[Kz] * uniformScaleXYZ);
	m_values[Kw] = (oldMatrix.m_values[Kw] * uniformScaleXYZ);
}

void Mat44::AppendScaleNonUniform2D(Vec2 const& nonUniformScaleXY)
{
	Mat44 oldMatrix(m_values);

	m_values[Ix] = (oldMatrix.m_values[Ix] * nonUniformScaleXY.x);
	m_values[Iy] = (oldMatrix.m_values[Iy] * nonUniformScaleXY.x);
	m_values[Iz] = (oldMatrix.m_values[Iz] * nonUniformScaleXY.x);
	m_values[Iw] = (oldMatrix.m_values[Iw] * nonUniformScaleXY.x);

	m_values[Jx] = (oldMatrix.m_values[Jx] * nonUniformScaleXY.y);
	m_values[Jy] = (oldMatrix.m_values[Jy] * nonUniformScaleXY.y);
	m_values[Jz] = (oldMatrix.m_values[Jz] * nonUniformScaleXY.y);
	m_values[Jw] = (oldMatrix.m_values[Jw] * nonUniformScaleXY.y);
}

void Mat44::AppendScaleNonUniform3D(Vec3 const& nonUniformScaleXYZ)
{
	Mat44 oldMatrix(m_values);

	m_values[Ix] = (oldMatrix.m_values[Ix] * nonUniformScaleXYZ.x);
	m_values[Iy] = (oldMatrix.m_values[Iy] * nonUniformScaleXYZ.x);
	m_values[Iz] = (oldMatrix.m_values[Iz] * nonUniformScaleXYZ.x);
	m_values[Iw] = (oldMatrix.m_values[Iw] * nonUniformScaleXYZ.x);

	m_values[Jx] = (oldMatrix.m_values[Jx] * nonUniformScaleXYZ.y);
	m_values[Jy] = (oldMatrix.m_values[Jy] * nonUniformScaleXYZ.y);
	m_values[Jz] = (oldMatrix.m_values[Jz] * nonUniformScaleXYZ.y);
	m_values[Jw] = (oldMatrix.m_values[Jw] * nonUniformScaleXYZ.y);

	m_values[Kx] = (oldMatrix.m_values[Kx] * nonUniformScaleXYZ.z);
	m_values[Ky] = (oldMatrix.m_values[Ky] * nonUniformScaleXYZ.z);
	m_values[Kz] = (oldMatrix.m_values[Kz] * nonUniformScaleXYZ.z);
	m_values[Kw] = (oldMatrix.m_values[Kw] * nonUniformScaleXYZ.z);
}

bool Mat44::operator==(Mat44 const& compare) const
{
	return (m_values[Ix] == compare.m_values[Ix] && m_values[Iy] == compare.m_values[Iy] && m_values[Iz] == compare.m_values[Iz] && m_values[Iw] == compare.m_values[Iw]
		&& m_values[Jx] == compare.m_values[Jx] && m_values[Jy] == compare.m_values[Jy] && m_values[Jz] == compare.m_values[Jz] && m_values[Jw] == compare.m_values[Jw]
		&& m_values[Kx] == compare.m_values[Kx] && m_values[Ky] == compare.m_values[Ky] && m_values[Kz] == compare.m_values[Kz] && m_values[Kw] == compare.m_values[Kw]
		&& m_values[Tx] == compare.m_values[Tx] && m_values[Ty] == compare.m_values[Ty] && m_values[Tz] == compare.m_values[Tz] && m_values[Tw] == compare.m_values[Tw]);
}

bool Mat44::operator!=(Mat44 const& compare) const
{
	return (m_values[Ix] != compare.m_values[Ix] || m_values[Iy] != compare.m_values[Iy] || m_values[Iz] != compare.m_values[Iz] || m_values[Iw] != compare.m_values[Iw]
		|| m_values[Jx] != compare.m_values[Jx] || m_values[Jy] != compare.m_values[Jy] || m_values[Jz] != compare.m_values[Jz] || m_values[Jw] != compare.m_values[Jw]
		|| m_values[Kx] != compare.m_values[Kx] || m_values[Ky] != compare.m_values[Ky] && m_values[Kz] != compare.m_values[Kz] || m_values[Kw] != compare.m_values[Kw]
		|| m_values[Tx] != compare.m_values[Tx] || m_values[Ty] != compare.m_values[Ty] || m_values[Tz] != compare.m_values[Tz] || m_values[Tw] != compare.m_values[Tw]);
}
