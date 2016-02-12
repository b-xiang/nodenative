#ifndef __NATIVE_FS_H__
#define __NATIVE_FS_H__

#include "base.h"
#include <functional>
#include <algorithm>
#include <fcntl.h>
#include "callback.h"
#include "error.h"

namespace native
{
    // TODO: implement functions that accept Loop pointer as extra argument.
    namespace fs
    {
        typedef uv_file file_handle;

        extern const int read_only;
        extern const int write_only;
        extern const int read_write;
        extern const int append;
        extern const int create;
        extern const int excl;
        extern const int truncate;
        extern const int no_follow;
        extern const int directory;
#ifdef O_NOATIME
        extern const int no_access_time;
#endif
#ifdef O_LARGEFILE
        extern const int large_large;
#endif

        namespace internal
        {
            template<typename callback_t>
            uv_fs_t* create_req(callback_t callback, void* data=nullptr)
            {
                auto req = new uv_fs_t;
                req->data = new callbacks(1);
                assert(req->data);
                callbacks::store(req->data, 0, callback, data);

                return req;
            }

            template<typename callback_t, typename ...A>
            typename std::result_of<callback_t(A...)>::type invoke_from_req(uv_fs_t* req, A&& ... args)
            {
                return callbacks::invoke<callback_t>(req->data, 0, std::forward<A>(args)...);
            }

            template<typename callback_t, typename data_t>
            data_t* get_data_from_req(uv_fs_t* req)
            {
                return reinterpret_cast<data_t*>(callbacks::get_data<callback_t>(req->data, 0));
            }

            template<typename callback_t, typename data_t>
            void delete_req_template(uv_fs_t* req)
            {
                delete reinterpret_cast<data_t*>(callbacks::get_data<callback_t>(req->data, 0));
                delete reinterpret_cast<callbacks*>(req->data);
                uv_fs_req_cleanup(req);
                delete req;
            }

            template<typename callback_t, typename data_t>
            void delete_req_arr_data(uv_fs_t* req)
            {
                delete[] reinterpret_cast<data_t*>(callbacks::get_data<callback_t>(req->data, 0));
                delete reinterpret_cast<callbacks*>(req->data);
                uv_fs_req_cleanup(req);
                delete req;
            }

            void delete_req(uv_fs_t* req);

            struct rte_context
            {
                fs::file_handle file;
                const static int buflen = 32;
                char buf[buflen];
                std::string result;
            };

            template<typename callback_t>
            void rte_cb(uv_fs_t* req)
            {
                assert(req->fs_type == UV_FS_READ);

                auto ctx = reinterpret_cast<rte_context*>(callbacks::get_data<callback_t>(req->data, 0));
                if(req->result < 0)
                {
                    // system error
                    error err(req->result);
                    invoke_from_req<callback_t>(req, std::string(), err);
                    delete_req_template<callback_t, rte_context>(req);
                }
                else if(req->result == 0)
                {
                    // EOF
                    error err;
                    invoke_from_req<callback_t>(req, ctx->result, err);
                    delete_req_template<callback_t, rte_context>(req);
                }
                else
                {
                    ctx->result.append(std::string(ctx->buf, req->result));

                    uv_fs_req_cleanup(req);
                    const uv_buf_t iov = uv_buf_init(ctx->buf, rte_context::buflen);

                    error err(uv_fs_read(uv_default_loop(), req, ctx->file, &iov, 1, ctx->result.length(), rte_cb<callback_t>));
                    if(err)
                    {
                        invoke_from_req<callback_t>(req, std::string(), err);
                        delete_req_template<callback_t, rte_context>(req);
                    }
                }
            }
        }

        bool open(const std::string& path, int flags, int mode, std::function<void(native::fs::file_handle fd, error e)> callback)
        {
            auto req = internal::create_req(callback);
            error err;
            if((err = uv_fs_open(uv_default_loop(), req, path.c_str(), flags, mode, [](uv_fs_t* req) {
                assert(req->fs_type == UV_FS_OPEN);

                if(req->result < 0) internal::invoke_from_req<decltype(callback)>(req, file_handle(-1), error(req->result));
                else internal::invoke_from_req<decltype(callback)>(req, req->result, error());

                internal::delete_req(req);
            }))) {
                // failed to initiate uv_fs_open()
                internal::delete_req(req);
                return false;
            }
            return true;
        }

        bool read(file_handle fd, size_t len, off_t offset, std::function<void(const std::string& str, error e)> callback)
        {
            auto buf = new char[len];
            auto req = internal::create_req(callback, buf);
            const uv_buf_t iov = uv_buf_init(buf, len);
            if(uv_fs_read(uv_default_loop(), req, fd, &iov, 1, offset, [](uv_fs_t* req){
                assert(req->fs_type == UV_FS_READ);

                if(req->result < 0)
                {
                    // system error
                    internal::invoke_from_req<decltype(callback)>(req, std::string(), error(req->result));
                }
                else if(req->result == 0)
                {
                    // EOF
                    internal::invoke_from_req<decltype(callback)>(req, std::string(), error(UV_EOF));
                }
                else
                {
                    auto buf = internal::get_data_from_req<decltype(callback), char>(req);
                    internal::invoke_from_req<decltype(callback)>(req, std::string(buf, req->result), error());
                }

                internal::delete_req_arr_data<decltype(callback), char>(req);
            })) {
                // failed to initiate uv_fs_read()
                internal::delete_req_arr_data<decltype(callback), char>(req);
                return false;
            }
            return true;
        }

        bool write(file_handle fd, const char* buf, size_t len, off_t offset, std::function<void(int nwritten, error e)> callback)
        {
            auto req = internal::create_req(callback);
            const uv_buf_t iov = uv_buf_init(const_cast<char*>(buf), len);

            // TODO: const_cast<> !!
            if(uv_fs_write(uv_default_loop(), req, fd, &iov, 1, offset, [](uv_fs_t* req){
                assert(req->fs_type == UV_FS_WRITE);

                if(req->result)
                {
                    internal::invoke_from_req<decltype(callback)>(req, 0, error(req->result));
                }
                else
                {
                    internal::invoke_from_req<decltype(callback)>(req, req->result, error());
                }

                internal::delete_req(req);
            })) {
                // failed to initiate uv_fs_write()
                internal::delete_req(req);
                return false;
            }
            return true;
        }

        bool read_to_end(file_handle fd, std::function<void(const std::string& str, error e)> callback)
        {
            auto ctx = new internal::rte_context;
            ctx->file = fd;
            auto req = internal::create_req(callback, ctx);
            const uv_buf_t iov = uv_buf_init(ctx->buf, internal::rte_context::buflen);

            if(uv_fs_read(uv_default_loop(), req, fd, &iov, 1, 0, internal::rte_cb<decltype(callback)>)) {
                // failed to initiate uv_fs_read()
                internal::delete_req_template<decltype(callback), internal::rte_context>(req);
                return false;
            }
            return true;
        }

        bool close(file_handle fd, std::function<void(error e)> callback)
        {
            auto req = internal::create_req(callback);
            if(uv_fs_close(uv_default_loop(), req, fd, [](uv_fs_t* req){
                assert(req->fs_type == UV_FS_CLOSE);
                internal::invoke_from_req<decltype(callback)>(req, req->result < 0?error(req->result):error());
                internal::delete_req(req);
            })) {
                internal::delete_req(req);
                return false;
            }
            return true;
        }

        bool unlink(const std::string& path, std::function<void(error e)> callback)
        {
            auto req = internal::create_req(callback);
            if(uv_fs_unlink(uv_default_loop(), req, path.c_str(), [](uv_fs_t* req){
                assert(req->fs_type == UV_FS_UNLINK);
                internal::invoke_from_req<decltype(callback)>(req, req->result < 0?error(req->result):error());
                internal::delete_req(req);
            })) {
                internal::delete_req(req);
                return false;
            }
            return true;
        }

        bool mkdir(const std::string& path, int mode, std::function<void(error e)> callback)
        {
            auto req = internal::create_req(callback);
            if(uv_fs_mkdir(uv_default_loop(), req, path.c_str(), mode, [](uv_fs_t* req){
                assert(req->fs_type == UV_FS_MKDIR);
                internal::invoke_from_req<decltype(callback)>(req, req->result < 0?error(req->result):error());
                internal::delete_req(req);
            })) {
                internal::delete_req(req);
                return false;
            }
            return true;
        }

        bool rmdir(const std::string& path, std::function<void(error e)> callback)
        {
            auto req = internal::create_req(callback);
            if(uv_fs_rmdir(uv_default_loop(), req, path.c_str(), [](uv_fs_t* req){
                assert(req->fs_type == UV_FS_RMDIR);
                internal::invoke_from_req<decltype(callback)>(req, req->result < 0?error(req->result):error());
                internal::delete_req(req);
            })) {
                internal::delete_req(req);
                return false;
            }
            return true;
        }

        bool rename(const std::string& path, const std::string& new_path, std::function<void(error e)> callback)
        {
            auto req = internal::create_req(callback);
            if(uv_fs_rename(uv_default_loop(), req, path.c_str(), new_path.c_str(), [](uv_fs_t* req){
                assert(req->fs_type == UV_FS_RENAME);
                internal::invoke_from_req<decltype(callback)>(req, req->result<0?error(req->result):error());
                internal::delete_req(req);
            })) {
                internal::delete_req(req);
                return false;
            }
            return true;
        }

        bool chmod(const std::string& path, int mode, std::function<void(error e)> callback)
        {
            auto req = internal::create_req(callback);
            if(uv_fs_chmod(uv_default_loop(), req, path.c_str(), mode, [](uv_fs_t* req){
                assert(req->fs_type == UV_FS_CHMOD);
                internal::invoke_from_req<decltype(callback)>(req, req->result<0?error(req->result):error());
                internal::delete_req(req);
            })) {
                internal::delete_req(req);
                return false;
            }
            return true;
        }

        bool chown(const std::string& path, int uid, int gid, std::function<void(error e)> callback)
        {
            auto req = internal::create_req(callback);
            if(uv_fs_chown(uv_default_loop(), req, path.c_str(), uid, gid, [](uv_fs_t* req){
                assert(req->fs_type == UV_FS_CHOWN);
                internal::invoke_from_req<decltype(callback)>(req, req->result<0?error(req->result):error());
                internal::delete_req(req);
            })) {
                internal::delete_req(req);
                return false;
            }
            return true;
        }

#if 0
        bool readdir(const std::string& path, int flags, std::function<void(error e)> callback)
        {
            auto req = internal::create_req(callback);
            if(uv_fs_readdir(uv_default_loop(), req, path.c_str(), flags, [](uv_fs_t* req){
                assert(req->fs_type == UV_FS_READDIR);
                internal::invoke_from_req<decltype(callback)>(req, req->result<0?error(req->result):error());
                internal::delete_req(req);
            })) {
                internal::delete_req(req);
                return false;
            }
            return true;
        }

        bool stat(const std::string& path, std::function<void(error e)> callback)
        {
            auto req = internal::create_req(callback);
            if(uv_fs_stat(uv_default_loop(), req, path.c_str(), [](uv_fs_t* req){
                assert(req->fs_type == UV_FS_STAT);
                internal::invoke_from_req<decltype(callback)>(req, req->result<0?error(req->result):error());
                internal::delete_req(req);
            })) {
                internal::delete_req(req);
                return false;
            }
            return true;
        }

        bool fstat(const std::string& path, std::function<void(error e)> callback)
        {
            auto req = internal::create_req(callback);
            if(uv_fs_fstat(uv_default_loop(), req, path.c_str(), [](uv_fs_t* req){
                assert(req->fs_type == UV_FS_FSTAT);
                internal::invoke_from_req<decltype(callback)>(req, req->result<0?error(req->result):error());
                internal::delete_req(req);
            })) {
                internal::delete_req(req);
                return false;
            }
            return true;
        }
#endif
    }

    class file
    {
    public:
        static bool read(const std::string& path, std::function<void(const std::string& str, error e)> callback)
        {
            return fs::open(path.c_str(), fs::read_only, 0, [=](fs::file_handle fd, error e) {
                if(e)
                {
                    callback(std::string(), e);
                }
                else
                {
                    if(!fs::read_to_end(fd, callback))
                    {
                        // failed to initiate read_to_end()
                        //TODO: this should not happen for async (callback provided). Temporary return unknown error. To resolve this.
                        //callback(std::string(), error(uv_last_error(uv_default_loop())));
                        callback(std::string(), error(UV__UNKNOWN));
                    }
                }
            });
        }

        static bool write(const std::string& path, const std::string& str, std::function<void(int nwritten, error e)> callback)
        {
            return fs::open(path.c_str(), fs::write_only|fs::create, 0664, [=](fs::file_handle fd, error e) {
                if(e)
                {
                    callback(0, e);
                }
                else
                {
                    if(!fs::write(fd, str.c_str(), str.length(), 0, callback))
                    {
                        // failed to initiate read_to_end()
                        //TODO: this should not happen for async (callback provided). Temporary return unknown error. To resolve this.
                        //callback(0, error(uv_last_error(uv_default_loop())));
                        callback(0, error(UV__UNKNOWN));
                    }
                }
            });
        }
    };
}

#endif // __NATIVE_FS_H__
