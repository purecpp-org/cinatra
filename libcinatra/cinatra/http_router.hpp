//
// Created by qiyu on 11/22/17.
//

#ifndef CINATRA_HTTP_ROUTER_HPP
#define CINATRA_HTTP_ROUTER_HPP

#include <vector>
#include <map>
#include <string>
#include <string_view>
#include <cinatra_http/request.h>
#include <cinatra_http/response.h>
#include "function_traits.hpp"
#include "iguana/json.hpp"

using namespace std::string_view_literals;

namespace cinatra{

    template <class T>
    struct sv_char_trait : std::char_traits<T> {
        using base_t = std::char_traits<T>;
        using char_type = typename base_t::char_type;

        static constexpr int compare(std::string_view s1, std::string_view s2) noexcept{
            if(s1.length()!=s2.length())
                return -1;

            size_t n = s1.length();
            for (size_t i = 0; i < n; ++i) {
                if (!base_t::eq(s1[i], s2[i])){
                    return base_t::eq(s1[i], s2[i]) ? -1 : 1;
                }
            }

            return 0;
        }

        static constexpr size_t find(std::string_view str, const char_type& a) noexcept {
            auto s = str.data();
            for (size_t i = 0; i < str.length(); ++i) {
                if (base_t::eq(s[i], a)){
                    return i;
                }
            }

            return  std::string_view::npos;
        }
    };

    template<typename T>
    constexpr bool  is_int64_v = std::is_same_v<T, std::int64_t>||std::is_same_v<T, std::uint64_t>;

    enum class httpmethod{
        DELETE,
        GET,
        HEAD,
        POST,
        PUT,
        CONNECT,
        OPTIONS,
        TRACE
    };
    constexpr inline auto GET = httpmethod::GET;
    constexpr inline auto POST = httpmethod::POST;
    constexpr inline auto DELETE = httpmethod::DELETE;
    constexpr inline auto HEAD = httpmethod::HEAD;
    constexpr inline auto PUT = httpmethod::PUT;
    constexpr inline auto CONNECT = httpmethod::CONNECT;
    constexpr inline auto TRACE = httpmethod::TRACE;
    constexpr inline auto OPTIONS = httpmethod::OPTIONS;

    constexpr auto type_to_name(std::integral_constant<httpmethod, httpmethod::DELETE>) noexcept { return "DELETE"sv; }
    constexpr auto type_to_name(std::integral_constant<httpmethod, httpmethod::GET>) noexcept { return "GET"sv; }
    constexpr auto type_to_name(std::integral_constant<httpmethod, httpmethod::HEAD>) noexcept { return "HEAD"sv; }

    constexpr auto type_to_name(std::integral_constant<httpmethod, httpmethod::POST>) noexcept { return "POST"sv; }
    constexpr auto type_to_name(std::integral_constant<httpmethod, httpmethod::PUT>) noexcept { return "PUT"sv; }

    constexpr auto type_to_name(std::integral_constant<httpmethod, httpmethod::CONNECT>) noexcept { return "CONNECT"sv; }
    constexpr auto type_to_name(std::integral_constant<httpmethod, httpmethod::OPTIONS>) noexcept { return "OPTIONS"sv; }
    constexpr auto type_to_name(std::integral_constant<httpmethod, httpmethod::TRACE>) noexcept { return "TRACE"sv; }

    class http_router
    {
    public:
        template<httpmethod... Is, typename Function, typename... Ap>
        void register_handler(std::string_view name,  Function&& f, Ap&&... ap) {
            if constexpr(sizeof...(Is)>0){
                auto arr=get_arr<Is...>(name);

                for(auto& s : arr){
                    register_nonmember_func(s, std::forward<Function>(f), std::forward<Ap>(ap)...);
                }
            }else{
                register_nonmember_func(std::string(name.data(), name.length()), std::forward<Function>(f), std::forward<Ap>(ap)...);
            }
        }

        template <httpmethod... Is, class T, class Type, typename T1, typename... Ap>
        void register_handler(std::string_view name,  Type T::* f, T1 t, Ap&&... ap) {
            register_handler_impl<Is...>(name, f, t, std::forward<Ap>(ap)...);
        }

        void remove_handler(std::string name) {
            this->map_invokers_.erase(name);
        }

        //elimate exception, resut type bool: true, success, false, failed
        bool route(std::string_view method, std::string_view url, const request& req, response& res)
        {
            std::string key(method.data(), method.length());
            key += std::string(url.data(), url.length());

            auto it = map_invokers_.find(key);
            if (it == map_invokers_.end()){
                return false;
            }

            it->second(req, res);
            return true;
        }

    private:
        template <httpmethod... Is, class T, class Type, typename T1, typename... Ap>
        void register_handler_impl(std::string_view name,  Type T::* f, T1&& t, Ap&&... ap) {
            if constexpr(sizeof...(Is)>0){
                auto arr=get_arr<Is...>(name);

                for(auto& s : arr){
                    register_member_func(s, f, std::forward<T>(t), std::forward<Ap>(ap)...);
                }
            }else{
                register_member_func(std::string(name.data(), name.length()), f, std::forward<T>(t), std::forward<Ap>(ap)...);
            }
        }

        template<typename Function, typename... Ap>
        void register_nonmember_func(const std::string& name,  Function&& f, Ap&&... ap) {
            this->map_invokers_[name] = [this, f = std::move(f), &ap...](const request& req, response& res){
                warper(std::make_tuple(ap...), [&f](const request& req1, response& res1)
                { return f(req1, res1); }, req, res);
            };
        }

        template<typename Function, typename Self, typename... Ap>
        void register_member_func(const std::string& name,  Function f, Self&& self, Ap&&... ap) {
            this->map_invokers_[name] = [this, f, &self, &ap...](const request& req, response& res){
                warper(std::make_tuple(ap...), [&f, &self](const request& req1, response& res1)
                { return (self.*f)(req1, res1); }, req, res);
            };
        }

        template<typename... AP, typename Function, typename... Args>
        auto warper(std::tuple<AP...> tp, Function&& f, Args&&... args){
            using result_type = typename timax::function_traits<Function>::result_type;

            //before
            bool r = true;
            for_each_l(tp, [&r, &args...](auto& item){
                if(!r)
                    return;

                if constexpr (has_before<decltype(item), Args...>::value)
                    r = item.before(std::forward<Args>(args)...);
            }, std::make_index_sequence<sizeof...(AP)>{});

            if constexpr(std::is_void_v<result_type>){
                if(!r)
                    return;
            }else{
                if(!r)
                    return result_type{};
            }

            //business
            auto lambda = [&f, &args...]{ return f(std::forward<Args>(args)...); };

            if constexpr(std::is_void_v<result_type>){
                lambda();
            }

            //after
            if constexpr(std::is_void_v<result_type>){
                for_each_r(tp, [&r, &args...](auto& item){
                    if(!r)
                        return;

                    if constexpr (has_after<decltype(item), std::string, Args...>::value)
                        r = item.after("", std::forward<Args>(args)...);
                }, std::make_index_sequence<sizeof...(AP)>{});
            }else{
                result_type result = lambda();
                std::string str = to_str(result);
                //translate the result to string
                for_each_r(tp, [&r, &str, &args...](auto& item){
                    if(!r)
                        return;

                    if constexpr (has_after<decltype(item), std::string, Args...>::value)
                        r = item.after(str, std::forward<Args>(args)...);
                }, std::make_index_sequence<sizeof...(AP)>{});
            }
        }

        template<typename T>
        std::string to_str(T&& value){
            using U = std::remove_const_t<std::remove_reference_t<T>>;
            if constexpr(std::is_integral_v<U>&&!is_int64_v<U>){
                std::vector<char> temp(20, '\0');
                itoa_fwd(value, temp.data());
                return std::string(temp.data());
            }
            else if constexpr (is_int64_v<U>){
                std::vector<char> temp(65, '\0');
                xtoa(value, temp.data(), 10, std::is_signed_v<U>);
                return std::string(temp.data());
            }
            else if constexpr (std::is_floating_point_v<U>){
                std::vector<char> temp(20, '\0');
                sprintf(temp.data(), "%f", value);
                return std::string(temp.data());
            }
            else if constexpr(std::is_same_v<std::string, U>){
                return value;
            }
            else {
                std::cout<<"this type has not supported yet"<<std::endl;
            }
        }

#define HAS_MEMBER(member)\
template<typename T, typename... Args>\
struct has_##member\
{\
private:\
    template<typename U> static auto Check(int) -> decltype(std::declval<U>().member(std::declval<Args>()...), std::true_type()); \
	template<typename U> static std::false_type Check(...);\
public:\
	enum{value = std::is_same<decltype(Check<T>(0)), std::true_type>::value};\
};

        HAS_MEMBER(before)
        HAS_MEMBER(after)

        template <typename... Args, typename F, std::size_t... Idx>
        constexpr void for_each_l(std::tuple<Args...>& t, F&& f, std::index_sequence<Idx...>)
        {
            (std::forward<F>(f)(std::get<Idx>(t)), ...);
        }

        template <typename... Args, typename F, std::size_t... Idx>
        constexpr void for_each_r(std::tuple<Args...>& t, F&& f, std::index_sequence<Idx...>)
        {
            constexpr auto size = sizeof...(Idx);
            (std::forward<F>(f)(std::get<size-Idx-1>(t)), ...);
        }

        template<httpmethod N>
        constexpr void get_str(std::string& s, std::string_view name){
            s = type_to_name(std::integral_constant<httpmethod, N>{}).data();
            s+=std::string(name.data(), name.length());
        }

        template<httpmethod... Is>
        constexpr auto get_arr(std::string_view name){
            std::array<std::string, sizeof...(Is)> arr={};
            size_t index = 0;
            (get_str<Is>(arr[index++], name), ...);

            return arr;
        }

        typedef std::function<void(const request&, response&)> invoker_function;
        std::map<std::string, invoker_function> map_invokers_;
    };
}

#endif //CINATRA_HTTP_ROUTER_HPP
