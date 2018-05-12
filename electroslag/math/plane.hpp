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
        class tplane {
        public:
            typedef tplane<T, P> type;
            typedef tplane<bool, P> bool_type;
            typedef T value_type;

            tplane()
                : m_point(0)
                , m_normal(0, 1, 0)
            {}

            tplane(type const& copy_object)
                : m_point(copy_object.m_point)
                , m_normal(copy_object.m_normal)
            {}

            template <class NewT, glm::precision NewP>
            tplane(tplane<NewT, NewP> const& copy_object)
                : m_point(copy_object.m_point)
                , m_normal(copy_object.m_normal)
            {}

            template <class NewT, glm::precision NewP>
            tplane(
                glm::tvec3<NewT, NewP> const& point,
                glm::tvec3<NewT, NewP> const& normal
                )
                : m_point(point)
                , m_normal(normalize(normal))
            {}

            tplane& operator =(tplane const& copy_object)
            {
                if (&copy_object != this) {
                    m_point = copy_object.m_point;
                    m_normal = copy_object.m_normal;
                }

                return (*this);
            }

            glm::tvec3<T, P> const& get_point() const
            {
                return (m_point);
            }

            glm::tvec3<T, P> const& get_normal() const
            {
                return (m_point);
            }

            void set(glm::tvec3<T, P> const& point, glm::tvec3<T, P> const& normal)
            {
                m_point = point;
                m_normal = normalize(normal);
            }

            void set(glm::tvec3<T, P> const& point_1, glm::tvec3<T, P> const& point_2, glm::tvec3<T, P> const& point_3)
            {
                m_point = point_1;

                // Assume right hand winding to get plane normal.
                glm::tvec3<T, P> vec_a(point_2 - point_1);
                glm::tvec3<T, P> vec_b(point_3 - point_2);

                m_normal = normalize(cross(vec_a, vec_b));
            }

            type transform(glm::tvec3<T, P> const& translate, glm::tquat<T, P> const& rotate) const
            {
                return (type(translate + m_point, rotate * m_normal));
            }

            value_type signed_distance(glm::tvec3<T, P> test_point) const
            {
                return (glm::dot(m_normal, (test_point - m_point)));
            }

        private:
            glm::tvec3<T, P> m_point;
            glm::tvec3<T, P> m_normal; // Normalized on init.
        };

        typedef tplane<float, glm::lowp> lowp_plane;
        typedef tplane<float, glm::lowp> lowp_fplane;

        typedef tplane<float, glm::mediump> mediump_plane;
        typedef tplane<float, glm::mediump> mediump_fplane;

        typedef tplane<float, glm::highp> highp_plane;
        typedef tplane<float, glm::highp> highp_fplane;

        typedef tplane<float, glm::defaultp> fplane;

#if(defined(GLM_PRECISION_LOWP_FLOAT))
        typedef lowp_plane plane;
#elif(defined(GLM_PRECISION_MEDIUMP_FLOAT))
        typedef mediump_plane plane;
#else //defined(GLM_PRECISION_HIGHP_FLOAT)
        typedef highp_plane plane;
#endif//GLM_PRECISION

        typedef tplane<glm::f32, glm::lowp>    lowp_f32plane;
        typedef tplane<glm::f32, glm::mediump> mediump_f32plane;
        typedef tplane<glm::f32, glm::highp>   highp_f32plane;
        typedef tplane<glm::f32, glm::defaultp> f32plane;
    }
}
