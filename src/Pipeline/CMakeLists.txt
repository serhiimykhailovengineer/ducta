add_library(ducta_pipeline STATIC
    Node.cpp
    Pipeline.cpp
)

set(BACKEND_LIBS)

if (DUCTA_USE_EMBEDDED_BACKEND)
    list(APPEND BACKEND_LIBS etl::etl)
else()
    list(APPEND BACKEND_LIBS Boost::boost fmt::fmt)
endif()

target_link_libraries(ducta_pipeline
    ducta_io
    ${BACKEND_LIBS}
)