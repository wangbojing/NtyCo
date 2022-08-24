



#include "nty_coroutine.h"


#include <mysql.h>


#define KING_DB_SERVER_IP		"192.168.233.133"
#define KING_DB_SERVER_PORT		3306

#define KING_DB_USERNAME		"king"
#define KING_DB_PASSWORD		"123456"

#define KING_DB_DEFAULTDB		"KING_DB"


#define SQL_INSERT_TBL_USER		"INSERT TBL_USER(U_NAME, U_GENDER) VALUES('King', 'man');"
#define SQL_SELECT_TBL_USER		"SELECT * FROM TBL_USER;"

#define SQL_DELETE_TBL_USER		"CALL PROC_DELETE_USER('King')"
#define SQL_INSERT_IMG_USER		"INSERT TBL_USER(U_NAME, U_GENDER, U_IMG) VALUES('King', 'man', ?);"

#define SQL_SELECT_IMG_USER		"SELECT U_IMG FROM TBL_USER WHERE U_NAME='King';"


#define FILE_IMAGE_LENGTH		(64*1024)
// C U R D --> 
// 

int king_mysql_select(MYSQL *handle) { //

	// mysql_real_query --> sql
	if (mysql_real_query(handle, SQL_SELECT_TBL_USER, strlen(SQL_SELECT_TBL_USER))) {
		printf("mysql_real_query : %s\n", mysql_error(handle));
		return -1;
	}
	

	// store --> 
	MYSQL_RES *res = mysql_store_result(handle);
	if (res == NULL) {
		printf("mysql_store_result : %s\n", mysql_error(handle));
		return -2;
	}

	// rows / fields
	int rows = mysql_num_rows(res);
	printf("rows: %d\n", rows);
	
	int fields = mysql_num_fields(res);
	printf("fields: %d\n", fields);

	// fetch
	MYSQL_ROW row;
	while ((row = mysql_fetch_row(res))) {

		int i = 0;
		for (i = 0;i < fields;i ++) {
			printf("%s\t", row[i]);
		}
		printf("\n");
		
	}

	mysql_free_result(res);

	return 0;
}


// filename : path + file name
// buffer : store image data

int read_image(char *filename, char *buffer) {

	if (filename == NULL || buffer == NULL) return -1;
	
	FILE *fp = fopen(filename, "rb"); //
	if (fp == NULL) {
		printf("fopen failed\n");
		return -2;
	}

	// file size
	fseek(fp, 0, SEEK_END);
	int length = ftell(fp); // file size
	fseek(fp, 0, SEEK_SET);

	int size = fread(buffer, 1, length, fp);
	if (size != length) {
		printf("fread failed: %d\n", size);
		return -3;
	}

	fclose(fp);

	return size;

}

// filename :
// buffer : 
// length :

int write_image(char *filename, char *buffer, int length) {

	if (filename == NULL || buffer == NULL || length <= 0) return -1;

	FILE *fp = fopen(filename, "wb+"); //
	if (fp == NULL) {
		printf("fopen failed\n");
		return -2;
	}

	int size = fwrite(buffer, 1, length, fp);
	if (size != length) {
		printf("fwrite failed: %d\n", size);
		return -3;
	}

	fclose(fp);

	return size;
}

int mysql_write(MYSQL *handle, char *buffer, int length) {

	if (handle == NULL || buffer == NULL || length <= 0) return -1;

	MYSQL_STMT *stmt = mysql_stmt_init(handle);
	int ret = mysql_stmt_prepare(stmt, SQL_INSERT_IMG_USER, strlen(SQL_INSERT_IMG_USER));
	if (ret) {
		printf("mysql_stmt_prepare : %s\n", mysql_error(handle));
		return -2;
	}

	MYSQL_BIND param = {0};
	param.buffer_type  = MYSQL_TYPE_LONG_BLOB;
	param.buffer = NULL;
	param.is_null = 0;
	param.length = NULL;

	ret = mysql_stmt_bind_param(stmt, &param);
	if (ret) {
		printf("mysql_stmt_bind_param : %s\n", mysql_error(handle));
		return -3;
	}

	ret = mysql_stmt_send_long_data(stmt, 0, buffer, length);
	if (ret) {
		printf("mysql_stmt_send_long_data : %s\n", mysql_error(handle));
		return -4;
	}

	ret = mysql_stmt_execute(stmt);
	if (ret) {
		printf("mysql_stmt_execute : %s\n", mysql_error(handle));
		return -5;
	}

	ret = mysql_stmt_close(stmt);
	if (ret) {
		printf("mysql_stmt_close : %s\n", mysql_error(handle));
		return -6;
	}
	

	return ret;
}

int mysql_read(MYSQL *handle, char *buffer, int length) {

	if (handle == NULL || buffer == NULL || length <= 0) return -1;

	MYSQL_STMT *stmt = mysql_stmt_init(handle);
	int ret = mysql_stmt_prepare(stmt, SQL_SELECT_IMG_USER, strlen(SQL_SELECT_IMG_USER));
	if (ret) {
		printf("mysql_stmt_prepare : %s\n", mysql_error(handle));
		return -2;
	}

	
	MYSQL_BIND result = {0};
	
	result.buffer_type  = MYSQL_TYPE_LONG_BLOB;
	unsigned long total_length = 0;
	result.length = &total_length;

	ret = mysql_stmt_bind_result(stmt, &result);
	if (ret) {
		printf("mysql_stmt_bind_result : %s\n", mysql_error(handle));
		return -3;
	}

	ret = mysql_stmt_execute(stmt);
	if (ret) {
		printf("mysql_stmt_execute : %s\n", mysql_error(handle));
		return -4;
	}

	ret = mysql_stmt_store_result(stmt);
	if (ret) {
		printf("mysql_stmt_store_result : %s\n", mysql_error(handle));
		return -5;
	}


	while (1) {

		ret = mysql_stmt_fetch(stmt);
		if (ret != 0 && ret != MYSQL_DATA_TRUNCATED) break; // 

		int start = 0;
		while (start < (int)total_length) {
			result.buffer = buffer + start;
			result.buffer_length = 1;
			mysql_stmt_fetch_column(stmt, &result, 0, start);
			start += result.buffer_length;
		}
	}

	mysql_stmt_close(stmt);

	return total_length;

}

void coroutine_func(void *arg) {

	MYSQL mysql;

	printf("coroutine_func\n");
	if (NULL == mysql_init(&mysql)) {
		printf("mysql_init : %s\n", mysql_error(&mysql));
		return ;
	}

	if (!mysql_real_connect(&mysql, KING_DB_SERVER_IP, KING_DB_USERNAME, KING_DB_PASSWORD, 
		KING_DB_DEFAULTDB, KING_DB_SERVER_PORT, NULL, 0)) {

		printf("mysql_real_connect : %s\n", mysql_error(&mysql));
		goto Exit;
	}

	// mysql --> insert 
	printf("case 1 : mysql --> insert \n");
#if 1
	if (mysql_real_query(&mysql, SQL_INSERT_TBL_USER, strlen(SQL_INSERT_TBL_USER))) {
		printf("mysql_real_query : %s\n", mysql_error(&mysql));
		goto Exit;
	}
#endif

	king_mysql_select(&mysql);

	// mysql --> delete 

	printf("case 2 : mysql --> delete \n");
#if 1
	if (mysql_real_query(&mysql, SQL_DELETE_TBL_USER, strlen(SQL_DELETE_TBL_USER))) {
		printf("mysql_real_query : %s\n", mysql_error(&mysql));
		goto Exit;
	}
#endif
	
	king_mysql_select(&mysql);



	printf("case 3 : mysql --> read image and write mysql\n");
	
	char buffer[FILE_IMAGE_LENGTH] = {0};
	int length = read_image("0voice.jpg", buffer);
	if (length < 0) goto Exit;
	
	mysql_write(&mysql, buffer, length); /// 


	printf("case 4 : mysql --> read mysql and write image\n");
	
	memset(buffer, 0, FILE_IMAGE_LENGTH);
	length = mysql_read(&mysql, buffer, FILE_IMAGE_LENGTH);

	write_image("a.jpg", buffer, length);

Exit:
	mysql_close(&mysql);

	return ;

}


int main() {
#if 1
	//init_hook();

	nty_coroutine *co = NULL;
	nty_coroutine_create(&co, coroutine_func, NULL);
	nty_schedule_run(); //run
#else

	coroutine_func(NULL);

#endif
}






