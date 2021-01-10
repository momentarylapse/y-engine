/*
 * NetworkManager.h
 *
 *  Created on: Jan 8, 2021
 *      Author: michi
 */

#ifndef SRC_NET_NETWORKMANAGER_H_
#define SRC_NET_NETWORKMANAGER_H_

#include "../lib/base/base.h"
#include "../lib/base/pointer.h"
#include <functional>

class Socket;
class BinaryBuffer;
namespace kaba {
	class Function;
}

class NetworkManager {
public:
	static void init();

	NetworkManager();

	struct Connection {
		owned<Socket> s;
		owned<BinaryBuffer> buffer;
		bool is_host;

		void start_block(const string &id);
		void end_block();
		void send();
		bool read_block();
	};
	Array<Connection*> connections;
	Connection *cur_con;

	//Socket *socket_to_host;

	Connection *connect_to_host(const string &host);
	void iterate();
	void iterate_client(Connection *con);
	void handle_block(Connection *con);

	void send_connect(Connection *con);

	//typedef std::function<void(VirtualBase*)> Callback;
	typedef void Callback(VirtualBase*);

	struct Observer {
		int hash;
		VirtualBase *object;
		Callback *callback;
	};
	Array<Observer> observers;
	void event(const string &message, VirtualBase *ob, Callback *cb);
	void event_kaba(const string &message, VirtualBase *ob, kaba::Function *f);
};

extern NetworkManager network_manager;

#endif /* SRC_NET_NETWORKMANAGER_H_ */
