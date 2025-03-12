include(FetchContent)

# Fetch googleapis
message(STATUS "protovalidate-cc: Fetching googleapis")
FetchContent_Declare(
    googleapis
    GIT_REPOSITORY https://github.com/googleapis/googleapis.git
    GIT_TAG 6eb56cdf5f54f70d0dbfce051add28a35c1203ce
)
FetchContent_MakeAvailable(googleapis)
