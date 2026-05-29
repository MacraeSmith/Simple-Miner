#pragma once
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/Vec3.hpp"
struct Vec2;
struct Vec4;

struct Mat44
{
public:
	enum {Ix, Iy, Iz, Iw,	Jx, Jy, Jz, Jw,		Kx, Ky, Kz, Kw,		Tx, Ty, Tz, Tw};
	float m_values[16] = { 1.f, 0.f, 0.f, 0.f,		0.f, 1.f, 0.f, 0.f,		0.f, 0.f, 1.f, 0.f,		0.f, 0.f, 0.f, 1.f };
	// laid out like this
	// [Ix	Jx	Kx	Tx]
	// [Iy	Jy	Ky	Ty]
	// [Iz	Jz	Kz	Tz]
	// [Iw	Jw	Kw	Tw]

	static const Mat44 IFWRD_JLEFT_KUP_TO_DX11RENDER;
	static const Mat44 IDENTITY;

	Mat44();
	explicit Mat44(Vec2 const& iBasis2D, Vec2 const& jBasis2D, Vec2 const& translation2D);
	explicit Mat44(Vec3 const& iBasis3D, Vec3 const& jBasis3D, Vec3 const& kBasis3D, Vec3 const& translation3D);
	explicit Mat44(Vec4 const& iBasis4D, Vec4 const& jBasis4D, Vec4 const& kBasis4D, Vec4 const& translation4D);
	explicit Mat44(Vec3 const& position, EulerAngles const& orientation, Vec3 const& scale = Vec3::ONE);
	explicit Mat44(float const* sixteenValuesBasisMajor);

	static Mat44 const	MakeTranslation2D(Vec2 const& translationXY);
	static Mat44 const	MakeTranslation3D(Vec3 const& translationXYZ);
	static Mat44 const	MakeUniformScale2D(float uniformScaleXY);
	static Mat44 const	MakeUniformScale3D(float unfiformScaleXYZ);
	static Mat44 const	MakeNonUniformScale2D(Vec2 const& nonUniformScaleXY);
	static Mat44 const	MakeNonUniformScale3D(Vec3 const& nonUniformScaleXYZ);
	static Mat44 const	MakeZRotationDegrees(float rotationDegreesAboutZ);
	static Mat44 const	MakeYRotationDegrees(float rotationDegreesAboutY);
	static Mat44 const	MakeXRotationDegrees(float rotationDegreesAboutX);
	static Mat44 const  MakeOrthoProjection(float left, float right, float bottom, float top, float zNear, float zFar);
	static Mat44 const  MakePerspectiveProjection(float fovYDegrees, float aspect, float zNear, float zFar);

	Vec2 const		TransformVectorQuantity2D(Vec2 const& vectorQuantityXY) const;		//assumes z=0 , w=0
	Vec3 const		TransformVectorQuantity3D(Vec3 const& vectorQuantityXYZ) const;		//asumes w=0
	Vec2 const		TransformPosition2D(Vec2 const& positionXY) const;					//assumes z=0, w=1
	Vec3 const		TransformPosition3D(Vec3 const& positionXYZ) const;					//assumes w=1
	Vec4 const		TransformHomogeneous3D(Vec4 const& homogenousPoint3D) const;			//w is provided

	float*			GetAsFloatArray();			//non-const (mutable) version
	float const*	GetAsFloatArray() const;	//const version used only when Mat44 is const
	Vec2 const		GetIBasis2D() const;
	Vec2 const		GetJBasis2D() const;
	Vec2 const		GetTranslation2D() const;
	Vec3 const		GetIBasis3D() const;
	Vec3 const		GetJBasis3D() const;
	Vec3 const		GetKBasis3D() const;
	Vec3 const		GetTranslation3D() const;
	Vec4 const		GetIBasis4D() const;
	Vec4 const		GetJBasis4D() const;
	Vec4 const		GetKBasis4D() const;
	Vec4 const		GetTranslation4D() const;
	Mat44 const		GetOrthonormalInverse() const;
	Mat44 const		GetInverse() const;
	Mat44 const		GetTransposed3x3() const;
	EulerAngles const GetOrientation() const;

	void	SetTranslation2D(Vec2 const& translationXY); //sets translationZ = 0, translationW = 1
	void	SetTranslation3D(Vec3 const& translationXYZ); //sets translationW =1
	void	SetIJ2D(Vec2 const& iBasis2D, Vec2 const& jBasis2D); //sets z=0, w=0 for i & j; does not modify k or t
	void	SetIJT2D(Vec2 const& iBasis2D, Vec2 const& jBasis2D, Vec2 const& translation2D); //sets z=0, w=0 for i & j; w=1 for t; does not modiy k
	void	SetIJK3D(Vec3 const& iBasis3D, Vec3 const& jBasis3D, Vec3 const& kBasis3D); //sets w=0 for ijk; does not modify t
	void	SetIJKT3D(Vec3 const& iBasis3D, Vec3 const& jBasis3D, Vec3 const& kBasis3D, Vec3 const& translationXYZ); //sets w=0 for ijk; w=1 for t
	void	SetIJKT4D(Vec4 const& iBasis4D, Vec4 const& jBasis4D, Vec4 const& kBasis4D, Vec4 const& translation4D); //All 16 values provided
	void	Transpose();
	void	Orthonormalize_IFwd_JLeft_KUp();
	void	Orthonormalize_IFwd_JRight_KUp();

	void Append(Mat44 const& appendThis);			//multiply  on right in column notation / on left in row notation
	void AppendZRotation(float degreesRotationAboutZ);				//same as appending (*= in column notation) a z-rotation matrix
	void AppendYRotation(float degreesRotationAboutY);				//same as appending (*= in column notation) a y-rotation matrix
	void AppendXRotation(float degreesRotationAboutX);				//same as appending (*= in column notation) a x-rotation matrix
	void AppendTranslation2D(Vec2 const& translationXY);			//same as appending (*= in column notation) a translation matrix
	void AppendTranslation3D(Vec3 const& translationXYZ);			//same as appending (*= in column notation) a translation matrix
	void AppendScaleUniform2D(float uniformScaleXY);				//K and T bases unaffected
	void AppendScaleUniform3D(float uniformScaleXYZ);				//T remains unaffected
	void AppendScaleNonUniform2D(Vec2 const& nonUniformScaleXY);	//K and T bases unaffected
	void AppendScaleNonUniform3D(Vec3 const& nonUniformScaleXYZ);	//T remains unaffected

	bool operator==(Mat44 const& compare)const;
	bool operator!=(Mat44 const& compare)const;


};

typedef Mat44 Matrix44;

