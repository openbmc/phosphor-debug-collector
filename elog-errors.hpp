// This file was autogenerated.  Do not edit!
// See elog-gen.py for more details
#pragma once

#include <string>
#include <tuple>
#include <type_traits>
#include <sdbusplus/exception.hpp>
#include <phosphor-logging/log.hpp>
#include <phosphor-logging/elog.hpp>

namespace sdbusplus
{
namespace xyz
{
namespace openbmc_project
{
namespace Dump
{
namespace Monitor
{
namespace Error
{
    struct InternalFailure;
} // namespace Error
} // namespace Monitor
} // namespace Dump
} // namespace openbmc_project
} // namespace xyz
} // namespace sdbusplus

namespace sdbusplus
{
namespace xyz
{
namespace openbmc_project
{
namespace Dump
{
namespace Monitor
{
namespace Error
{
    struct InvalidCorePath;
} // namespace Error
} // namespace Monitor
} // namespace Dump
} // namespace openbmc_project
} // namespace xyz
} // namespace sdbusplus


namespace phosphor
{

namespace logging
{

namespace example
{
namespace xyz
{
namespace openbmc_project
{
namespace Example
{
namespace Device
{
namespace _Callout
{

struct CALLOUT_ERRNO_TEST
{
    static constexpr auto str = "CALLOUT_ERRNO_TEST=%d";
    static constexpr auto str_short = "CALLOUT_ERRNO_TEST";
    using type = std::tuple<std::decay_t<decltype(str)>,int32_t>;
    explicit constexpr CALLOUT_ERRNO_TEST(int32_t a) : _entry(entry(str, a)) {};
    type _entry;
};
struct CALLOUT_DEVICE_PATH_TEST
{
    static constexpr auto str = "CALLOUT_DEVICE_PATH_TEST=%s";
    static constexpr auto str_short = "CALLOUT_DEVICE_PATH_TEST";
    using type = std::tuple<std::decay_t<decltype(str)>,const char*>;
    explicit constexpr CALLOUT_DEVICE_PATH_TEST(const char* a) : _entry(entry(str, a)) {};
    type _entry;
};

}  // namespace _Callout

struct Callout : public sdbusplus::exception_t
{
    static constexpr auto errName = "example.xyz.openbmc_project.Example.Device.Callout";
    static constexpr auto errDesc = "Generic device callout";
    static constexpr auto L = level::INFO;
    using CALLOUT_ERRNO_TEST = _Callout::CALLOUT_ERRNO_TEST;
    using CALLOUT_DEVICE_PATH_TEST = _Callout::CALLOUT_DEVICE_PATH_TEST;
    using metadata_types = std::tuple<CALLOUT_ERRNO_TEST, CALLOUT_DEVICE_PATH_TEST>;

    const char* name() const noexcept
    {
        return errName;
    }

    const char* description() const noexcept
    {
        return errDesc;
    }

    const char* what() const noexcept
    {
        return errName;
    }
};

} // namespace Device
} // namespace Example
} // namespace openbmc_project
} // namespace xyz
} // namespace example



namespace xyz
{
namespace openbmc_project
{
namespace Dump
{
namespace Monitor
{
namespace _InternalFailure
{

struct FAIL
{
    static constexpr auto str = "FAIL=%s";
    static constexpr auto str_short = "FAIL";
    using type = std::tuple<std::decay_t<decltype(str)>,const char*>;
    explicit constexpr FAIL(const char* a) : _entry(entry(str, a)) {};
    type _entry;
};
struct ERRNO
{
    static constexpr auto str = "ERRNO=%s";
    static constexpr auto str_short = "ERRNO";
    using type = std::tuple<std::decay_t<decltype(str)>,const char*>;
    explicit constexpr ERRNO(const char* a) : _entry(entry(str, a)) {};
    type _entry;
};

}  // namespace _InternalFailure

struct InternalFailure : public sdbusplus::exception_t
{
    static constexpr auto errName = "xyz.openbmc_project.Dump.Monitor.InternalFailure";
    static constexpr auto errDesc = "The operation failed internally during Core file monitoring.";
    static constexpr auto L = level::ERR;
    using FAIL = _InternalFailure::FAIL;
    using ERRNO = _InternalFailure::ERRNO;
    using metadata_types = std::tuple<FAIL, ERRNO>;

    const char* name() const noexcept
    {
        return errName;
    }

    const char* description() const noexcept
    {
        return errDesc;
    }

    const char* what() const noexcept
    {
        return errName;
    }
};

} // namespace Monitor
} // namespace Dump
} // namespace openbmc_project
} // namespace xyz


namespace details
{

template <>
struct map_exception_type<sdbusplus::xyz::openbmc_project::Dump::Monitor::Error::InternalFailure>
{
    using type = xyz::openbmc_project::Dump::Monitor::InternalFailure;
};

}

namespace xyz
{
namespace openbmc_project
{
namespace Dump
{
namespace Monitor
{
namespace _InvalidCorePath
{

struct PATH
{
    static constexpr auto str = "PATH=%s";
    static constexpr auto str_short = "PATH";
    using type = std::tuple<std::decay_t<decltype(str)>,const char*>;
    explicit constexpr PATH(const char* a) : _entry(entry(str, a)) {};
    type _entry;
};

}  // namespace _InvalidCorePath

struct InvalidCorePath : public sdbusplus::exception_t
{
    static constexpr auto errName = "xyz.openbmc_project.Dump.Monitor.InvalidCorePath";
    static constexpr auto errDesc = "Invalid core directory path";
    static constexpr auto L = level::ERR;
    using PATH = _InvalidCorePath::PATH;
    using metadata_types = std::tuple<PATH>;

    const char* name() const noexcept
    {
        return errName;
    }

    const char* description() const noexcept
    {
        return errDesc;
    }

    const char* what() const noexcept
    {
        return errName;
    }
};

} // namespace Monitor
} // namespace Dump
} // namespace openbmc_project
} // namespace xyz


namespace details
{

template <>
struct map_exception_type<sdbusplus::xyz::openbmc_project::Dump::Monitor::Error::InvalidCorePath>
{
    using type = xyz::openbmc_project::Dump::Monitor::InvalidCorePath;
};

}

namespace example
{
namespace xyz
{
namespace openbmc_project
{
namespace Example
{
namespace Elog
{
namespace _TestErrorTwo
{

struct DEV_ADDR
{
    static constexpr auto str = "DEV_ADDR=0x%.8X";
    static constexpr auto str_short = "DEV_ADDR";
    using type = std::tuple<std::decay_t<decltype(str)>,uint32_t>;
    explicit constexpr DEV_ADDR(uint32_t a) : _entry(entry(str, a)) {};
    type _entry;
};
struct DEV_ID
{
    static constexpr auto str = "DEV_ID=%u";
    static constexpr auto str_short = "DEV_ID";
    using type = std::tuple<std::decay_t<decltype(str)>,uint32_t>;
    explicit constexpr DEV_ID(uint32_t a) : _entry(entry(str, a)) {};
    type _entry;
};
struct DEV_NAME
{
    static constexpr auto str = "DEV_NAME=%s";
    static constexpr auto str_short = "DEV_NAME";
    using type = std::tuple<std::decay_t<decltype(str)>,const char*>;
    explicit constexpr DEV_NAME(const char* a) : _entry(entry(str, a)) {};
    type _entry;
};

}  // namespace _TestErrorTwo

struct TestErrorTwo : public sdbusplus::exception_t
{
    static constexpr auto errName = "example.xyz.openbmc_project.Example.Elog.TestErrorTwo";
    static constexpr auto errDesc = "This is test error two";
    static constexpr auto L = level::ERR;
    using DEV_ADDR = _TestErrorTwo::DEV_ADDR;
    using DEV_ID = _TestErrorTwo::DEV_ID;
    using DEV_NAME = _TestErrorTwo::DEV_NAME;
    using metadata_types = std::tuple<DEV_ADDR, DEV_ID, DEV_NAME>;

    const char* name() const noexcept
    {
        return errName;
    }

    const char* description() const noexcept
    {
        return errDesc;
    }

    const char* what() const noexcept
    {
        return errName;
    }
};

} // namespace Elog
} // namespace Example
} // namespace openbmc_project
} // namespace xyz
} // namespace example



namespace example
{
namespace xyz
{
namespace openbmc_project
{
namespace Example
{
namespace Elog
{
namespace _AutoTestSimple
{

struct STRING
{
    static constexpr auto str = "STRING=%s";
    static constexpr auto str_short = "STRING";
    using type = std::tuple<std::decay_t<decltype(str)>,const char*>;
    explicit constexpr STRING(const char* a) : _entry(entry(str, a)) {};
    type _entry;
};

}  // namespace _AutoTestSimple

struct AutoTestSimple : public sdbusplus::exception_t
{
    static constexpr auto errName = "example.xyz.openbmc_project.Example.Elog.AutoTestSimple";
    static constexpr auto errDesc = "This is a simple test error.";
    static constexpr auto L = level::ERR;
    using STRING = _AutoTestSimple::STRING;
    using metadata_types = std::tuple<STRING>;

    const char* name() const noexcept
    {
        return errName;
    }

    const char* description() const noexcept
    {
        return errDesc;
    }

    const char* what() const noexcept
    {
        return errName;
    }
};

} // namespace Elog
} // namespace Example
} // namespace openbmc_project
} // namespace xyz
} // namespace example



namespace example
{
namespace xyz
{
namespace openbmc_project
{
namespace Example
{
namespace Elog
{
namespace _TestCallout
{

struct DEV_ADDR
{
    static constexpr auto str = "DEV_ADDR=0x%.8X";
    static constexpr auto str_short = "DEV_ADDR";
    using type = std::tuple<std::decay_t<decltype(str)>,uint32_t>;
    explicit constexpr DEV_ADDR(uint32_t a) : _entry(entry(str, a)) {};
    type _entry;
};

}  // namespace _TestCallout

struct TestCallout : public sdbusplus::exception_t
{
    static constexpr auto errName = "example.xyz.openbmc_project.Example.Elog.TestCallout";
    static constexpr auto errDesc = "This is test error TestCallout";
    static constexpr auto L = level::ERR;
    using DEV_ADDR = _TestCallout::DEV_ADDR;
    using CALLOUT_ERRNO_TEST = example::xyz::openbmc_project::Example::Device::Callout::CALLOUT_ERRNO_TEST;
    using CALLOUT_DEVICE_PATH_TEST = example::xyz::openbmc_project::Example::Device::Callout::CALLOUT_DEVICE_PATH_TEST;
    using metadata_types = std::tuple<DEV_ADDR, CALLOUT_ERRNO_TEST, CALLOUT_DEVICE_PATH_TEST>;

    const char* name() const noexcept
    {
        return errName;
    }

    const char* description() const noexcept
    {
        return errDesc;
    }

    const char* what() const noexcept
    {
        return errName;
    }
};

} // namespace Elog
} // namespace Example
} // namespace openbmc_project
} // namespace xyz
} // namespace example



namespace example
{
namespace xyz
{
namespace openbmc_project
{
namespace Example
{
namespace Elog
{
namespace _TestErrorOne
{

struct ERRNUM
{
    static constexpr auto str = "ERRNUM=0x%.4X";
    static constexpr auto str_short = "ERRNUM";
    using type = std::tuple<std::decay_t<decltype(str)>,uint16_t>;
    explicit constexpr ERRNUM(uint16_t a) : _entry(entry(str, a)) {};
    type _entry;
};
struct FILE_PATH
{
    static constexpr auto str = "FILE_PATH=%s";
    static constexpr auto str_short = "FILE_PATH";
    using type = std::tuple<std::decay_t<decltype(str)>,const char*>;
    explicit constexpr FILE_PATH(const char* a) : _entry(entry(str, a)) {};
    type _entry;
};
struct FILE_NAME
{
    static constexpr auto str = "FILE_NAME=%s";
    static constexpr auto str_short = "FILE_NAME";
    using type = std::tuple<std::decay_t<decltype(str)>,const char*>;
    explicit constexpr FILE_NAME(const char* a) : _entry(entry(str, a)) {};
    type _entry;
};

}  // namespace _TestErrorOne

struct TestErrorOne : public sdbusplus::exception_t
{
    static constexpr auto errName = "example.xyz.openbmc_project.Example.Elog.TestErrorOne";
    static constexpr auto errDesc = "this is test error one";
    static constexpr auto L = level::INFO;
    using ERRNUM = _TestErrorOne::ERRNUM;
    using FILE_PATH = _TestErrorOne::FILE_PATH;
    using FILE_NAME = _TestErrorOne::FILE_NAME;
    using DEV_ADDR = example::xyz::openbmc_project::Example::Elog::TestErrorTwo::DEV_ADDR;
    using DEV_ID = example::xyz::openbmc_project::Example::Elog::TestErrorTwo::DEV_ID;
    using DEV_NAME = example::xyz::openbmc_project::Example::Elog::TestErrorTwo::DEV_NAME;
    using metadata_types = std::tuple<ERRNUM, FILE_PATH, FILE_NAME, DEV_ADDR, DEV_ID, DEV_NAME>;

    const char* name() const noexcept
    {
        return errName;
    }

    const char* description() const noexcept
    {
        return errDesc;
    }

    const char* what() const noexcept
    {
        return errName;
    }
};

} // namespace Elog
} // namespace Example
} // namespace openbmc_project
} // namespace xyz
} // namespace example



namespace example
{
namespace xyz
{
namespace openbmc_project
{
namespace Example
{
namespace Foo
{
namespace _Foo
{

struct FOO_DATA
{
    static constexpr auto str = "FOO_DATA=%s";
    static constexpr auto str_short = "FOO_DATA";
    using type = std::tuple<std::decay_t<decltype(str)>,const char*>;
    explicit constexpr FOO_DATA(const char* a) : _entry(entry(str, a)) {};
    type _entry;
};

}  // namespace _Foo

struct Foo : public sdbusplus::exception_t
{
    static constexpr auto errName = "example.xyz.openbmc_project.Example.Foo.Foo";
    static constexpr auto errDesc = "this is test error Foo";
    static constexpr auto L = level::INFO;
    using FOO_DATA = _Foo::FOO_DATA;
    using ERRNUM = example::xyz::openbmc_project::Example::Elog::TestErrorOne::ERRNUM;
    using FILE_PATH = example::xyz::openbmc_project::Example::Elog::TestErrorOne::FILE_PATH;
    using FILE_NAME = example::xyz::openbmc_project::Example::Elog::TestErrorOne::FILE_NAME;
    using DEV_ADDR = example::xyz::openbmc_project::Example::Elog::TestErrorTwo::DEV_ADDR;
    using DEV_ID = example::xyz::openbmc_project::Example::Elog::TestErrorTwo::DEV_ID;
    using DEV_NAME = example::xyz::openbmc_project::Example::Elog::TestErrorTwo::DEV_NAME;
    using metadata_types = std::tuple<FOO_DATA, ERRNUM, FILE_PATH, FILE_NAME, DEV_ADDR, DEV_ID, DEV_NAME>;

    const char* name() const noexcept
    {
        return errName;
    }

    const char* description() const noexcept
    {
        return errDesc;
    }

    const char* what() const noexcept
    {
        return errName;
    }
};

} // namespace Foo
} // namespace Example
} // namespace openbmc_project
} // namespace xyz
} // namespace example



namespace example
{
namespace xyz
{
namespace openbmc_project
{
namespace Example
{
namespace Bar
{
namespace _Bar
{

struct BAR_DATA
{
    static constexpr auto str = "BAR_DATA=%s";
    static constexpr auto str_short = "BAR_DATA";
    using type = std::tuple<std::decay_t<decltype(str)>,const char*>;
    explicit constexpr BAR_DATA(const char* a) : _entry(entry(str, a)) {};
    type _entry;
};

}  // namespace _Bar

struct Bar : public sdbusplus::exception_t
{
    static constexpr auto errName = "example.xyz.openbmc_project.Example.Bar.Bar";
    static constexpr auto errDesc = "this is test error Bar";
    static constexpr auto L = level::INFO;
    using BAR_DATA = _Bar::BAR_DATA;
    using FOO_DATA = example::xyz::openbmc_project::Example::Foo::Foo::FOO_DATA;
    using ERRNUM = example::xyz::openbmc_project::Example::Elog::TestErrorOne::ERRNUM;
    using FILE_PATH = example::xyz::openbmc_project::Example::Elog::TestErrorOne::FILE_PATH;
    using FILE_NAME = example::xyz::openbmc_project::Example::Elog::TestErrorOne::FILE_NAME;
    using DEV_ADDR = example::xyz::openbmc_project::Example::Elog::TestErrorTwo::DEV_ADDR;
    using DEV_ID = example::xyz::openbmc_project::Example::Elog::TestErrorTwo::DEV_ID;
    using DEV_NAME = example::xyz::openbmc_project::Example::Elog::TestErrorTwo::DEV_NAME;
    using metadata_types = std::tuple<BAR_DATA, FOO_DATA, ERRNUM, FILE_PATH, FILE_NAME, DEV_ADDR, DEV_ID, DEV_NAME>;

    const char* name() const noexcept
    {
        return errName;
    }

    const char* description() const noexcept
    {
        return errDesc;
    }

    const char* what() const noexcept
    {
        return errName;
    }
};

} // namespace Bar
} // namespace Example
} // namespace openbmc_project
} // namespace xyz
} // namespace example




} // namespace logging

} // namespace phosphor
