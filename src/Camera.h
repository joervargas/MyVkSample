#pragma once

#include <algorithm>
#include "UtilsMath.h"

#include <glm/glm.hpp>
#include "glm/gtx/euler_angles.hpp"
#include <glm/ext.hpp>
using glm::quat;
using glm::mat4;
using glm::vec4;
using glm::vec3;
using glm::vec2;


class CameraPositionInterface
{
public:

    virtual ~CameraPositionInterface() = default;
    virtual mat4 getViewMatrix() const = 0;
    virtual vec3 getPosition() const = 0;

};

class Camera final
{
public:

    explicit Camera(CameraPositionInterface& positioner) :
        m_positioner(&positioner)
    {}

    Camera(const Camera&) = default;

    Camera& operator = (const Camera&) = default;

    mat4 getViewMatrix() const { return m_positioner->getViewMatrix(); }

    vec3 getPosition() const { return m_positioner->getPosition(); }

private:

    const CameraPositionInterface* m_positioner;
};

class CameraPositioner_FirstPerson final : public CameraPositionInterface
{
public:

    struct Movement
    {
        bool forward = false;
        bool backward = false;
        bool left = false;
        bool right = false;
        bool up = false;
        bool down = false;
        bool fastSpeed = false;
    } movement;

    float m_mouseSpeed = 4.0f;
    float m_acceleration = 150.0f;
    float m_damping = 0.2f;
    float m_maxSpeed = 10.0f;
    float m_fastCoef = 10.0f;

    CameraPositioner_FirstPerson() = default;
    CameraPositioner_FirstPerson(const vec3& pos, const vec3& target, const vec3& up) :
        m_cameraPosition(pos),
        m_cameraOrientation(glm::lookAt(pos, target, up))
    {}

    void update(double deltaSeconds, const vec2& mousePos, bool mousePressed)
    {
        if(mousePressed)
        {
            const vec2 delta = mousePos - m_mousePos;
            const quat deltaQuat = quat(vec3(m_mouseSpeed * delta.y, m_mouseSpeed * delta.x, 0.0f));
            m_cameraOrientation = glm::normalize(deltaQuat * m_cameraOrientation);
        }
        m_mousePos = mousePos;

        const mat4 v = glm::mat4_cast(m_cameraOrientation);

        const vec3 forward = -vec3(v[0][2], v[1][2], v[2][2]);
        const vec3 right = vec3(v[0][0], v[1][0], v[2][0]);
        const vec3 up = glm::cross(right, forward);

        vec3 accel(0.0f);
        if(movement.forward) accel += forward;
        if(movement.backward) accel -= forward;
        if(movement.right) accel += right;
        if(movement.left) accel -= right;
        if(movement.up) accel += up;
        if(movement.down) accel -= up;
        if(movement.fastSpeed) accel *= m_fastCoef;

        // decelerate camera movment gradually
        if(accel == vec3(0)) // Camera's acceleration is 0
        {
            m_moveSpeed -= m_mouseSpeed * std::min( (1.0f/ m_damping) * static_cast<float>(deltaSeconds), 1.0f);
        }
        else // Camera's acceleration is not 0
        {
            m_moveSpeed += accel * m_acceleration * static_cast<float>(deltaSeconds);
            const float maxSpeed = movement.fastSpeed ? m_maxSpeed * m_fastCoef : m_maxSpeed;

            if(glm::length(m_moveSpeed) > maxSpeed)
            {
                m_moveSpeed = glm::normalize(m_moveSpeed) * maxSpeed;
            }
        }

        m_cameraPosition += m_moveSpeed * static_cast<float>(deltaSeconds);
    }

    virtual mat4 getViewMatrix() const override
    {
        const mat4 t = glm::translate(glm::mat4(1.0f), -m_cameraPosition);
        const mat4 r = glm::mat4_cast(m_cameraOrientation);
        return r * t;
    }

    virtual vec3 getPosition() const override { return m_cameraPosition; }
    void setPosition(const vec3& pos) { m_cameraPosition = pos; }

    void setUpVector(const vec3& up)
    {
        const mat4 view = getViewMatrix();
        const vec3 dir = -glm::vec3(view[0][2], view[1][2], view[2][2]);
        m_cameraOrientation = glm::lookAt(m_cameraPosition, m_cameraPosition + dir, up);
    }

    void resetMousePosition(const vec2& p)
    {
        m_mousePos = p;
    }

private:

    vec2 m_mousePos = vec2(0);
    vec3 m_cameraPosition = vec3(0.0f, 10.0f, 10.0f);
    quat m_cameraOrientation = quat(vec3(0));
    vec3 m_moveSpeed = vec3(0.0f);
};

class CameraPositioner_MoveTo final : public CameraPositionInterface
{
public:

    CameraPositioner_MoveTo(const vec3& pos, const vec3& angles) :
        m_positionCurrent(pos),
        m_positionDesired(pos),
        m_anglesCurrent(angles),
        m_anglesDesired(angles)
    {}

    void update(float deltaSeconds, const glm::vec2& mousePos, bool mousePressed)
    {
        m_positionCurrent += dampingLinear * deltaSeconds * (m_positionDesired - m_positionCurrent);

        m_anglesCurrent = clipAngles(m_anglesCurrent);
        m_anglesDesired = clipAngles(m_anglesDesired);

        m_anglesCurrent -= angleDelta(m_anglesCurrent, m_anglesDesired) * dampingEulerAngles * deltaSeconds;

        m_anglesCurrent = clipAngles(m_anglesCurrent);

        const vec3 a = glm::radians(m_anglesCurrent);

        m_currentTransform = glm::translate(glm::yawPitchRoll(a.y, a.x, a.z), -m_positionCurrent);
    }

    void setPosition(const vec3& p) { m_positionCurrent = p; }
    void setAngles(float pitch, float pan, float roll) { m_anglesCurrent = vec3(pitch, pan, roll); }
    void setAngles(const vec3& angles) { m_anglesCurrent = angles; }
    void setDesiredPosition(const vec3& p) { m_positionDesired = p; }
    void setDesiredAngles(float pitch, float pan, float roll) { m_anglesDesired = vec3(pitch, pan, roll); }
    void setDesiredAngles(const vec3& angles) { m_anglesDesired = angles; }

    virtual vec3 getPosition() const override { return m_positionCurrent; }
    virtual mat4 getViewMatrix() const override { return m_currentTransform; }

public:

    float dampingLinear = 10.0f;
    vec3 dampingEulerAngles = vec3(5.0f, 5.0f, 5.0f);

private:

    vec3 m_positionCurrent = vec3(0.0f);
    vec3 m_positionDesired = vec3(0.0f);

    vec3 m_anglesCurrent = vec3(0.0f);
    vec3 m_anglesDesired = vec3(0.0f);

    mat4 m_currentTransform = mat4(1.0f);

    static inline float clipAngle(float d)
    {
        if( d < -180.0f) { return d + 360.0f; }
        if( d > 180.0f) { return d - 360.0f; }
        return d;
    }

    static inline vec3 clipAngles(const vec3& angles)
    {
        return vec3(
            std::fmod(angles.x, 360.0f),
            std::fmod(angles.y, 360.0f),
            std::fmod(angles.z, 360.0f)
        );
    }

    static inline glm::vec3 angleDelta(const vec3& anglesCurrent, const vec3& anglesDesired)
    {
        const vec3 d = clipAngles(anglesCurrent) - clipAngles(anglesDesired);
        return vec3(clipAngle(d.x), clipAngle(d.y), clipAngle(d.z));
    }
};