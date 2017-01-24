#pragma once

#include <cinatra_http/request.h>
#include <cinatra_http/response.h>

#include <vector>
#include <memory>

#if defined(_WIN32)
#define __SWAP_LONGLONG(l)            \
            ( ( ((l) >> 56) & 0x00000000000000FFLL ) |       \
              ( ((l) >> 40) & 0x000000000000FF00LL ) |       \
              ( ((l) >> 24) & 0x0000000000FF0000LL ) |       \
              ( ((l) >>  8) & 0x00000000FF000000LL ) |       \
              ( ((l) <<  8) & 0x000000FF00000000LL ) |       \
              ( ((l) << 24) & 0x0000FF0000000000LL ) |       \
              ( ((l) << 40) & 0x00FF000000000000LL ) |       \
              ( ((l) << 56) & 0xFF00000000000000LL ) )

inline uint64_t htobe64 (uint64_t val)
{
	const uint64_t ret = __SWAP_LONGLONG (val);
	return ret;
}

inline uint64_t be64toh (uint64_t val)
{
	const uint64_t ret = __SWAP_LONGLONG (val);
	return ret;
}

#elif defined(__APPLE__)
#include <libkern/OSByteOrder.h>
#define be64toh(x) OSSwapBigToHostInt64(x)
#define htobe64(x) OSSwapHostToBigInt64(x)
#endif //_WIN32

namespace cinatra
{
	namespace websocket
	{

		using ws_conn_ptr_t = std::shared_ptr<class websocket_connection>;
		using upgrade_callback_t = std::function<void(ws_conn_ptr_t)>;

		enum opcode_t : unsigned char
		{
			TEXT = 1,
			BINARY = 2,
			CLOSE = 8,
			PING = 9,
			PONG = 10
		};

		struct ws_config_t
		{
			std::function<void(ws_conn_ptr_t, boost::string_ref, opcode_t)> on_message;
			std::function<void(ws_conn_ptr_t, boost::string_ref)> on_ping;
			std::function<void(ws_conn_ptr_t, boost::string_ref)> on_pong;
			std::function<void(ws_conn_ptr_t, boost::string_ref, opcode_t)> on_close;
			std::function<void(boost::system::error_code const&)> on_error;
		};

		using async_write_msg_callback_t = std::function<void(boost::system::error_code const&)>;
		using async_send_close_callback_t = async_write_msg_callback_t;
		class websocket_connection : public std::enable_shared_from_this<websocket_connection>
		{
		public:
			websocket_connection() = delete;
			websocket_connection(std::shared_ptr<response::connection> conn, ws_config_t cfg);
			~websocket_connection()
			{

			}
			static boost::string_ref is_websocket_handshake(const request& req);
			static void upgrade_to_websocket(request const& req, response& rep, boost::string_ref const& client_key, ws_config_t cfg);


			void start();

			void async_send_msg(std::string const& text, opcode_t opCode, async_write_msg_callback_t handler)
			{
				async_send_msg(text.data(), text.size(), opCode, std::move(handler));
			}

			void async_send_msg(const char* data, std::size_t length, opcode_t opCode, async_write_msg_callback_t handler);

			void close(int code, char *message, size_t length);

		private:
			enum state_t
			{
				READ_HEAD,
				READ_MESSAGE
			};

			enum send_state_t
			{
				SND_CONTINUATION = 1,
				SND_NO_FIN = 2,
				SND_COMPRESSED = 64
			};

			struct close_frame_t
			{
				uint16_t code;
				char *message;
				size_t length;
			};

			using frame_format_t = uint16_t;
			static inline bool rsv1(frame_format_t &frame) { return (frame & 64) != 0; }
			static inline bool rsv23(frame_format_t &frame) { return (frame & 48) != 0; }
			static inline unsigned char get_opcode(frame_format_t &frame) { return frame & 15; }
			static inline unsigned char payload_length(frame_format_t &frame) { return (frame >> 8) & 127; }
			static inline bool is_fin(frame_format_t &frame) { return (frame & 128) != 0; }
			static inline bool refuse_payload_length(void *user, std::size_t length) { return length > 16777216; }

			static void unmask_imprecise(char *dst, char *src, char *mask, std::size_t length);
			static void unmask_imprecise_copy_mask(char *dst, char *src, char *maskPtr, std::size_t length);
			static void unmask_inplace(char *data, char *stop, char *mask);
			static void rotate_mask(unsigned int offset, char *mask);
			static close_frame_t parse_close_payload(char *src, size_t length);


			bool set_compressed(void *user) { return false; }	//TODO:���gzip֧��
			void force_close(void *user) { conn_->close(); }

			void consume(char *src, std::size_t length, void *user);

			template <const int MESSAGE_HEADER, typename T>
			inline bool consume_message(T pay_length, char *&src, std::size_t &length, frame_format_t frame, void *user)
			{
				if (get_opcode(frame))
				{
					if (op_stack == 1 || (!last_fin && get_opcode(frame) < 2))
					{
						force_close(user);
						return true;
					}
					opcode[(unsigned char) ++op_stack] = (opcode_t)get_opcode(frame);
				}
				else if (op_stack == -1)
				{
					force_close(user);
					return true;
				}
				last_fin = is_fin(frame);

				if (refuse_payload_length(user, static_cast<std::size_t>(pay_length)))
				{
					force_close(user);
					return true;
				}

				if (int(pay_length) <= int(length - MESSAGE_HEADER))
				{
					
					unmask_imprecise_copy_mask(src, src + MESSAGE_HEADER, src + MESSAGE_HEADER - 4, static_cast<std::size_t>(pay_length));
					if (handle_fragment(src, static_cast<std::size_t>(pay_length), 0, opcode[(unsigned char)op_stack], is_fin(frame), user))
					{
						return true;
					}

					if (is_fin(frame))
					{
						op_stack--;
					}

					src += pay_length + MESSAGE_HEADER;
					length -= static_cast<std::size_t>(pay_length + MESSAGE_HEADER);
					spill_length = 0;
					return false;
				}
				else
				{
					spill_length = 0;
					state = READ_MESSAGE;
					remaining_bytes = static_cast<std::size_t>(pay_length - length + MESSAGE_HEADER);

					memcpy(mask, src + MESSAGE_HEADER - 4, 4);
					unmask_imprecise(src, src + MESSAGE_HEADER, mask, length);
					rotate_mask(4 - (length - MESSAGE_HEADER) % 4, mask);
					handle_fragment(src, length - MESSAGE_HEADER, remaining_bytes, opcode[(unsigned char)op_stack], is_fin(frame), user);
					return true;
				}
			}

			bool consume_continuation(char *&src, std::size_t &length, void *user);

			bool handle_fragment(char *data, size_t length, std::size_t remaining_bytes, int opcode, bool fin, void *user);

			std::vector<boost::asio::const_buffer> format_message(const char *src, size_t length, opcode_t opcode/*, size_t reportedLength, bool compressed*/);


			static size_t format_close_payload(char *dst, uint16_t code, char *message, size_t length);

			bool is_closed()
			{
				return conn_->is_closed();
			}

			bool is_shutting_down()
			{
				return shutting_down;
			}

			std::shared_ptr<response::connection> conn_;
			std::vector<char> buffer_;
			static const int SHORT_MESSAGE_HEADER = 6;
			static const int MEDIUM_MESSAGE_HEADER = 8;
			static const int LONG_MESSAGE_HEADER = 14;

			// this can hold two states (1 bit)
			// this can hold length of spill (up to 16 = 4 bit)
			unsigned char state = READ_HEAD;
			std::size_t spill_length = 0; // remove this!
			char op_stack = -1; // remove this too
			char last_fin = true; // hold in state!
			unsigned char spill[LONG_MESSAGE_HEADER - 1];
			std::size_t remaining_bytes = 0;
			char mask[4];
			opcode_t opcode[2];

			char msg_header_[10];

			std::string fragment_buffer;
			std::string control_buffer;

			bool shutting_down = false;

			ws_config_t cfg_;
		};


	}

}