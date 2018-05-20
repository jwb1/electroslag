//  Electroslag Interactive Graphics System
//  Copyright 2018 Joshua Buckman
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.

#pragma once

namespace electroslag {
    namespace math {
        template <class T, glm::precision P = glm::defaultp>
        class taabb {
        public:
            typedef taabb<T, P> type;
            typedef taabb<bool, P> bool_type;
            typedef T value_type;

            taabb()
                : m_min_corner(std::numeric_limits<T>::max())
                , m_max_corner(std::numeric_limits<T>::min())
            {}

            taabb(type const& copy_object)
                : m_min_corner(copy_object.m_min_corner)
                , m_max_corner(copy_object.m_max_corner)
            {}

            template <class NewT, glm::precision NewP>
            taabb(taabb<NewT, NewP> const& copy_object)
                : m_min_corner(copy_object.m_min_corner)
                , m_max_corner(copy_object.m_max_corner)
            {}

            explicit taabb(value_type const& scalar)
                : m_min_corner(scalar)
                , m_max_corner(scalar)
            {}

            template <class NewT, glm::precision NewP>
            taabb(
                glm::tvec3<NewT, NewP> const& min_corner,
                glm::tvec3<NewT, NewP> const& max_corner
                )
                : m_min_corner(min_corner)
                , m_max_corner(max_corner)
            {}

            taabb(
                value_type const& min_x, value_type const& min_y, value_type const& min_z,
                value_type const& max_x, value_type const& max_y, value_type const& max_z
                )
                : m_min_corner(min_x, min_y, min_z)
                , m_max_corner(max_x, max_y, max_z)
            {}

            template <class NewT, glm::precision NewP>
            taabb(glm::tvec3<NewT, NewP> const* vert_array, int vert_count, int stride = sizeof(glm::tvec3<NewT, NewP>))
                : m_min_corner(std::numeric_limits<T>::max())
                , m_max_corner(std::numeric_limits<T>::min())
            {
                for (int i = 0; i < vert_count; ++i) {
                    add_vert(vert_array);
                    vert_array = static_cast<glm::tvec3<T, P> const*>(static_cast<byte const*>(vert_array) + stride);
                }
            }

            taabb& operator =(taabb const& copy_object)
            {
                if (&copy_object != this) {
                    m_min_corner = copy_object.m_min_corner;
                    m_max_corner = copy_object.m_max_corner;
                }

                return (*this);
            }

            glm::tvec3<T, P> const& get_min_corner() const
            {
                return (m_min_corner);
            }

            glm::tvec3<T, P> const& get_max_corner() const
            {
                return (m_max_corner);
            }

            template <class NewT, glm::precision NewP>
            void add_vert(glm::tvec3<NewT, NewP> const& vert)
            {
                m_min_corner = glm::min(m_min_corner, vert);
                m_max_corner = glm::max(m_max_corner, vert);
            }

            template <class NewT, glm::precision NewP>
            void translate(glm::tvec3<NewT, NewP> const& t)
            {
                m_min_corner += t;
                m_max_corner += t;
            }

        private:
            glm::tvec3<T, P> m_min_corner;
            glm::tvec3<T, P> m_max_corner;
        };

        template<class T, glm::precision P>
        taabb<T, P> operator *(taabb<T, P> const& aabb, glm::tmat4x4<T, P> const& m)
        {
            // http://dev.theomader.com/transform-bounding-boxes/
            glm::tvec3<T, P> const& min_corner = aabb.get_min_corner();
            glm::tvec3<T, P> const& max_corner = aabb.get_max_corner();

            typename glm::tmat4x4<T, P>::col_type const& right       = m[0];
            typename glm::tmat4x4<T, P>::col_type const& up          = m[1];
            typename glm::tmat4x4<T, P>::col_type const& backward    = m[2];
            typename glm::tmat4x4<T, P>::col_type const& translation = m[3];

            glm::tvec3<T, P> xa(right * min_corner.x);
            glm::tvec3<T, P> xb(right * max_corner.x);

            glm::tvec3<T, P> ya(up * min_corner.y);
            glm::tvec3<T, P> yb(up * max_corner.y);

            glm::tvec3<T, P> za(backward * min_corner.z);
            glm::tvec3<T, P> zb(backward * max_corner.z);

            return (taabb<T, P>(
                glm::min(xa, xb) + glm::min(ya, yb) + glm::min(za, zb) + glm::tvec3<T, P>(translation),
                glm::max(xa, xb) + glm::max(ya, yb) + glm::max(za, zb) + glm::tvec3<T, P>(translation)
                ));
        }

        typedef taabb<float, glm::lowp> lowp_aabb;
        typedef taabb<float, glm::lowp> lowp_faabb;

        typedef taabb<float, glm::mediump> mediump_aabb;
        typedef taabb<float, glm::mediump> mediump_faabb;

        typedef taabb<float, glm::highp> highp_aabb;
        typedef taabb<float, glm::highp> highp_faabb;

        typedef taabb<float, glm::defaultp> faabb;

#if(defined(GLM_PRECISION_LOWP_FLOAT))
        typedef lowp_aabb aabb;
#elif(defined(GLM_PRECISION_MEDIUMP_FLOAT))
        typedef mediump_aabb aabb;
#else //defined(GLM_PRECISION_HIGHP_FLOAT)
        typedef highp_aabb aabb;
#endif//GLM_PRECISION

        typedef taabb<glm::f32, glm::lowp>    lowp_f32aabb;
        typedef taabb<glm::f32, glm::mediump> mediump_f32aabb;
        typedef taabb<glm::f32, glm::highp>   highp_f32aabb;
        typedef taabb<glm::f32, glm::defaultp> f32aabb;
    }
}
