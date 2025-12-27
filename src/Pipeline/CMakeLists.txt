add_library(ducta_pipeline STATIC
    Node.cpp
    Pipeline.cpp
)

target_link_libraries(ducta_pipeline
    ducta_io
    Boost::boost
)