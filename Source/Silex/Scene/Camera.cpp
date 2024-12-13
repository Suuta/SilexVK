
#include "PCH.h"
#include "Scene/Camera.h"


namespace Silex
{
    Camera::Camera(glm::vec3 position)
    {
        Position         = position;
        Front            = { 0.0f, 0.0f, 1.0f };
        WorldUp          = { 0.0f, 1.0f, 0.0f };

        MovementSpeed    = 10.0f;
        MouseSensitivity = 0.1f;

        Yaw              = 0.0f;
        Pitch            = 0.0f;
        FOV              = 60.0f;

        UpdateCameraAxisVectors();

        View = glm::lookAt(Position, Position + Front, Up);
        Projection = glm::perspectiveFov(glm::radians(FOV), ViwpoerSize.x, ViwpoerSize.y, NearPlane, FarPlane);
    }

    void Camera::Update(float deltaTime)
    {
        View       = glm::lookAt(Position, Position + Front, Up);
        Projection = glm::perspectiveFov(glm::radians(FOV), ViwpoerSize.x, ViwpoerSize.y, NearPlane, FarPlane);
    }

    void Camera::SetViewportSize(uint32 width, uint32 height)
    {
        ViwpoerSize.x = (float)width;
        ViwpoerSize.y = (float)height;
    }

    void Camera::Move(CameraMovementDir direction, float deltaTime)
    {
        float velocity = MovementSpeed * deltaTime;

        if (direction == CameraMovementDir::Forward)  Position += Front   * velocity;
        if (direction == CameraMovementDir::Backward) Position -= Front   * velocity;
        if (direction == CameraMovementDir::Left)     Position -= Right   * velocity;
        if (direction == CameraMovementDir::Right)    Position += Right   * velocity;
        if (direction == CameraMovementDir::Up)       Position += WorldUp * velocity;
        if (direction == CameraMovementDir::Down)     Position -= WorldUp * velocity;
    }

    void Camera::ProcessMouseMovement(float xoffset, float yoffset)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw   += xoffset;
        Pitch += yoffset;
        Pitch  = std::clamp(Pitch, -89.0f, 89.0f);

        UpdateCameraAxisVectors();
    }

    void Camera::UpdateCameraAxisVectors()
    {
        glm::vec3 front;

        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));

        Front = glm::normalize(front);
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up    = glm::normalize(glm::cross(Right, Front));
    }

    void Camera::ProcessMouseScroll(float offset)
    {
        Position += Front * offset * 1.5f;
    }
}
