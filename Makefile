objects = tmp/main.o tmp/util.o tmp/api.o tmp/sessions.o \
	tmp/urlparser.o tmp/requesthandler.o tmp/listener.o tmp/certificate.o \
	tmp/indexpage.o tmp/basicapi.o tmp/auth.o tmp/cipher.o
flags = -std=c++17 -g -I ./header/
libs = -lstdc++fs -lssl -lcrypto -ldl -lmysqlcppconn -pthread -lfmt -lspdlog

server: $(objects)
	g++ -o server $(objects) $(libs) $(flags)

tmp/%.o : src/%.cpp
	g++ -c -o $@ $< $(flags)

.PHONY : clean
clean :
	rm server $(objects)