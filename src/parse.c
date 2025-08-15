#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "common.h"
#include "parse.h"


int edit_employees(struct dbheader_t *dbhdr, struct employee_t *employees, char *editstring) {
	char *name = strtok(editstring, ",");

	char *hours = strtok(NULL, ",");

	int i = 0;
	for (; i < dbhdr->count; i++) {
		if(strcmp(employees[i].name, name) == 0) {
			employees[i].hours = atoi(hours);
		}
	}

	return STATUS_SUCCESS;
}

int delete_employees(struct dbheader_t *dbhdr, struct employee_t *employees, char *deletestring) {
	int i, j;
    int found = 0;

    for (i = 0; i < dbhdr->count; ) {
        if (strcmp(employees[i].name, deletestring) == 0) {
            found = 1;

            for (j = i; j < dbhdr->count - 1; j++) {
                employees[j] = employees[j + 1];
            }

            memset(&employees[dbhdr->count - 1], 0, sizeof(struct employee_t));

            dbhdr->count--;
        } else {
            i++;
        }
    }

    if (found) {
        printf("All employees with the name '%s' deleted successfully.\n", deletestring);
    } else {
        printf("No employees with the name '%s' were found.\n", deletestring);
    }
    
	return STATUS_SUCCESS;
}

int list_employees(struct dbheader_t *dbhdr, struct employee_t *employees) {
	int i = 0;
	for (; i < dbhdr->count; i++) {
		printf("Employee %d\n", i);
		printf("\tName: %s\n", employees[i].name);
		printf("\tAddress: %s\n", employees[i].address);
		printf("\tHours: %d\n", employees[i].hours);
	}
	return STATUS_SUCCESS;
}

int add_employee(struct dbheader_t *dbhdr, struct employee_t **employees, char *addstring) {
	if (!dbhdr || !employees || !addstring) return STATUS_ERROR;
	if (dbhdr->count == 0) return STATUS_ERROR;
	char *name = strtok(addstring, ",");

	char *addr = strtok(NULL, ",");

	char *hours = strtok(NULL, ",");

	strncpy((*employees)[dbhdr->count-1].name, name, sizeof((*employees)[dbhdr->count-1].name));
	strncpy((*employees)[dbhdr->count-1].address, addr, sizeof((*employees)[dbhdr->count-1].address));

	(*employees)[dbhdr->count-1].hours = atoi(hours);

	return STATUS_SUCCESS;
}

int read_employees(int fd, struct dbheader_t *dbhdr, struct employee_t **employeesOut) {
 	if (fd < 0) {
                printf("Got a bad FD from the user\n");
                return STATUS_ERROR;
        }

	int count = dbhdr->count;

	struct employee_t *employees = calloc(count, sizeof(struct employee_t));
	if (employees == NULL) {
		printf("Malloc failed\n");
		return STATUS_ERROR;
	}

	read(fd, employees, count*sizeof(struct employee_t));

	int i = 0;
	for (;i < count; i++) {
		employees[i].hours = ntohl(employees[i].hours);
	}

	*employeesOut = employees;
        return STATUS_SUCCESS;

}

int output_file(int fd, struct dbheader_t *dbhdr, struct employee_t *employees) {
	if (fd < 0) {
                printf("Got a bad FD from the user\n");
                return STATUS_ERROR;
        }

	int realcount = dbhdr->count;
	
	dbhdr->magic = htonl(dbhdr->magic);
	dbhdr->filesize = htonl(sizeof(struct dbheader_t) + (sizeof(struct employee_t) * realcount));
	dbhdr->count = htons(dbhdr->count);
	dbhdr->version = htons(dbhdr->version);

	lseek(fd, 0, SEEK_SET);

	write(fd, dbhdr, sizeof(struct dbheader_t));

	int i = 0;
	for (; i < realcount; i++) {
		employees[i].hours = htonl(employees[i].hours);
		write(fd, &employees[i], sizeof(struct employee_t));
	}

	off_t new_size = sizeof(struct dbheader_t) + (sizeof(struct employee_t) * realcount);
	if (ftruncate(fd, new_size) != 0) {
        perror("Failed to truncate file");
        return STATUS_ERROR;
    }

	return STATUS_SUCCESS;
}	

int validate_db_header(int fd, struct dbheader_t **headerOut) {
	if (fd < 0) {
		printf("Got a bad FD from the user\n");
		return STATUS_ERROR;
	}

	struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
	if (header == NULL) {
		printf("Malloc failed to create db header\n");
		return STATUS_ERROR;
	}

	if (read(fd, header, sizeof(struct dbheader_t)) != sizeof(struct dbheader_t)) {
		perror("read");
		free(header);
		return STATUS_ERROR;
	}

	header->version = ntohs(header->version);
	header->count = ntohs(header->count);
	header->magic = ntohl(header->magic);
	header->filesize = ntohl(header->filesize);

	if (header->magic != HEADER_MAGIC) {
		printf("Improper header magic\n");
		free(header);
		return STATUS_ERROR;
	}

	if (header->version != 1) {
                printf("Improper header version\n");
                free(header);
                return STATUS_ERROR;
        }

	struct stat dbstat = {0};
	fstat(fd, &dbstat);
	if (header->filesize != dbstat.st_size) {
		printf("Corrupted database\n");
		free(header);
		return STATUS_ERROR;
	}

	*headerOut = header;

}

int create_db_header(struct dbheader_t **headerOut) {
	if (headerOut == NULL) {
        return STATUS_ERROR;
    }
	struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
	if (header == NULL) {
		printf("Malloc failed to create db header\n");
		return STATUS_ERROR;
	}
	
	header->version = 0x1;
	header->count = 0;
	header->magic = HEADER_MAGIC;
	header->filesize = sizeof *header;
	
	*headerOut = header;

	return STATUS_SUCCESS;
}
