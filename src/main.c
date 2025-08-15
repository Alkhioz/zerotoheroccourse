#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include <stdlib.h>

#include "common.h"
#include "file.h"
#include "parse.h"

void print_usage(char *argv[]) {
	printf("Usage: %s -n -f <database file>\n", argv[0]);
	printf("\t -n	- create new database file\n");
	printf("\t -f	- (required) path to database file\n");
	return;
}

int main(int argc, char *argv[]) { 
	char *filepath = NULL;
	char *addstring = NULL;
	bool newfile = false;
	bool list = false;
	char *editstring = NULL;
	char *deletestring = NULL;
	int *deleted_employees = 0;
	int c;

	int dbfd = -1;
	struct dbheader_t *dbhdr = NULL;
	struct employee_t *employees = NULL;

	while ((c = getopt(argc, argv, "nf:a:le:d:")) != -1) {
		switch (c) {
			case 'n':
				newfile = true;
				break;
			case 'f':
				filepath = optarg;
				break;
			case 'a':
				addstring = optarg;
				break;
			case 'l':
				list = true;
				break;
			case 'd':
				deletestring = optarg;
				break;
			case 'e':
				editstring = optarg;
				break;
			case '?':
				printf("Unknowm option %c\n", c);
				break;
			default:
				return -1;
		}
	}

	if (filepath == NULL) {
		printf("Filepath is a required argument\n");
		print_usage(argv);

		return 0;
	}

	if (newfile) {
		dbfd = create_db_file(filepath);
		if (dbfd == STATUS_ERROR) {
			printf("Unable to create database file\n");
			return -1;
		}

		if (create_db_header(&dbhdr) == STATUS_ERROR) {
			printf("Failed to create database header");
			return -1;
		}

	} else {
		dbfd = open_db_file(filepath);
		 if (dbfd == STATUS_ERROR) {
                        printf("Unable to open database file\n");
                        return -1;
                }

		if (validate_db_header(dbfd, &dbhdr) == STATUS_ERROR) {
			printf("Failed to validate database header\n");
			return -1;
		}
	}

	if (read_employees(dbfd, dbhdr, &employees) != STATUS_SUCCESS) {
		printf("Failed to read employees\n");
		return 0;
	}

	if (addstring) {
		dbhdr->count++;
		struct employee_t *tmp = realloc(employees, dbhdr->count*(sizeof(struct employee_t)));
		int status = add_employee(dbhdr, &tmp, addstring);
		if (status == -1) {
			dbhdr->count--;
		} else {
			employees = tmp;
		}
	}

	if(deletestring) {
		delete_employees(dbhdr, employees, deletestring);
		employees = realloc(employees, dbhdr->count*(sizeof(struct employee_t)));
	}

	if (editstring) {
		edit_employees(dbhdr, employees, editstring);
	}

	if (list) {
		list_employees(dbhdr, employees);
	}

	output_file(dbfd, dbhdr, employees);

	return 0;	
}
