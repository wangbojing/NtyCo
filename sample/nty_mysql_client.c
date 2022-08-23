




#include "nty_coroutine.h"

#include <stdio.h>
#include <string.h>
#include <mysql.h>


void func (void *arg) {

	
	MYSQL* m_mysql = mysql_init(NULL);
	if (!m_mysql) {
		printf("mysql_init failed\n");
		return ;
	}

	if (!mysql_real_connect(m_mysql, 
                "192.168.233.133", "king", "123456",
                "KING_DB", 3306,
                NULL, CLIENT_FOUND_ROWS)) {
		printf("mysql_real_connect failed: %s\n", mysql_error(m_mysql));
		return ;
	} else{
		printf("mysql_real_connect success\n");
	}

	
}

int main() {
#if 1
	init_hook();

	nty_coroutine *co = NULL;
	nty_coroutine_create(&co, func, NULL);
	nty_schedule_run(); //run
#else

	func(NULL);

#endif
}


