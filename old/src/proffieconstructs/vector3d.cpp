#include "vector3d.h"
/*
 * ProffieConfig, All-In-One Proffieboard Management Utility
 * Copyright (C) 2024 Ryan Ogurek,
 * partially based on code from ProffieOS, copyright Fredrik Hubinette et al.
 *
 * proffieconstructs/vector3d.cpp
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <cmath>

Vector3D::Vector3D() : x(0), y(0), z(0) {}
Vector3D::Vector3D(float val) : x(val), y(val), z(val) {}
Vector3D::Vector3D(float xVal, float yVal, float zVal) : x(xVal), y(yVal), z(zVal) {}

Vector3D Vector3D::operator+(const Vector3D& operand) const {
    return {x + operand.x, y + operand.y, z + operand.z};
}

Vector3D Vector3D::operator+(float operand) const {
    return {x + operand, y + operand, z + operand};
}

Vector3D Vector3D::operator-(const Vector3D& operand) const {
    return {x - operand.x, y - operand.y, z - operand.z};
}

Vector3D Vector3D::operator-(float operand) const {
    return {x - operand, y - operand, z - operand};
}

Vector3D Vector3D::operator-() const {
    return {-x, -y, -z};
}

Vector3D Vector3D::operator*(const Vector3D& operand) const {
    return {x * operand.x, y * operand.y, z * operand.z};
}

Vector3D Vector3D::operator*(float operand) const {
    return {x * operand, y * operand, z * operand};
}
Vector3D Vector3D::operator/(const Vector3D& operand) const {
    return {x / operand.x, y / operand.y, z / operand.z};
}

Vector3D Vector3D::operator/(float operand) const {
    return {x / operand, y / operand, z / operand};
}

Vector3D Vector3D::operator+=(const Vector3D& operand) {
    x += operand.x;
    y += operand.y;
    z += operand.z;
    return *this;
}

Vector3D Vector3D::operator-=(const Vector3D& operand) {
    x -= operand.x;
    y -= operand.y;
    z -= operand.z;
    return *this;
}

Vector3D Vector3D::operator*=(const Vector3D& operand) {
    x *= operand.x;
    y *= operand.y;
    z *= operand.z;
    return *this;
}

Vector3D Vector3D::operator/=(const Vector3D& operand) {
    x /= operand.x;
    y /= operand.y;
    z /= operand.z;
    return *this;
}

float Vector3D::dot(const Vector3D& operand) const {
    return (x * operand.x) + (y * operand.y) + (z * operand.z);
}

Vector3D Vector3D::cross(const Vector3D& operand) const {
    return {
            (y * operand.z) - (z * operand.y),
            (z * operand.x) - (x * operand.z),
            (x * operand.y) - (y * operand.x)
            };
}

// Length squared
float Vector3D::length2() const {
    return (x * x) + (y * y) + (z * z);
}
float Vector3D::length() const {
    return std::sqrt(length2());
}

void Vector3D::rotate90(float& axisA, float& axisB) {
    auto tmp{axisB};
    axisB = -axisA;
    axisA = tmp;
}

void Vector3D::rotate180(float& axisA, float& axisB) {
    axisA = -axisA;
    axisB = -axisB;
}

// Rotate about the given axis
void Vector3D::rotateX90() {
    rotate90(y, z);
}

void Vector3D::rotateY90() {
    rotate90(x, y);
}

void Vector3D::rotateZ90() {
    rotate90(z, x);
}

void Vector3D::rotateX180() {
    rotate180(y, z);
}

void Vector3D::rotateY180() {
    rotate180(x, y);
}

void Vector3D::rotateZ180() {
    rotate180(z, x);
}

Vector3D Vector3D::rotateX(float angle) const {
    float sin{std::sin(angle)};
    float cos{std::cos(angle)};
    return {
            x, 
            (y * cos) + (z * sin), 
            (y * -sin) + (z * cos)
        };
}

Vector3D Vector3D::rotateY(float angle) const {
    float sin{std::sin(angle)};
    float cos{std::cos(angle)};
    return {
            (x * cos) - (z * sin),
            y,
            (y * sin) + (z * cos)
        };
}

Vector3D Vector3D::rotateZ(float angle) const {
    float sin{std::sin(angle)};
    float cos{std::cos(angle)};
    return {
            (x * cos) - (y * sin),
            (x * sin) + (y * cos),
            z
        };
}

Vector3D Vector3D::rotate(float angle) const {
    float sin{std::sin(angle)};
    float cos{std::cos(angle)};
    
    auto newX{(x * cos * cos) - (z * sin * cos) - (y * sin)};
    auto newY{(x * sin) + (y * cos * cos) + (z * sin * cos)};
    auto newZ{(y * sin) + (y * -sin * cos) + (z * cos * cos)};

    return {newX, newY, newZ};
}

Vector3D Vector3D::moveTowardsZero(float delta) const {
    auto oldLength{length()};
    auto newLength{oldLength - delta};
    if (newLength <= 0.F) return {0.F};
    return (*this) * (newLength / oldLength);
}


