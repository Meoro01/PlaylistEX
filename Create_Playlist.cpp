#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#define MAX_ELEMENT 200
#define MAX_TITLE 100
#define MAX_GENRE 50
#define MAX_GENRE_WEIGHTS 20


// å ����ü
typedef struct {
    char title[MAX_TITLE];
    char author[MAX_TITLE];
    char genre[MAX_GENRE];
} Book;

// �뷡 ����ü
typedef struct {
    char title[MAX_TITLE];
    char artist[MAX_TITLE];
    char genres[2][MAX_GENRE]; // �帣 2�� ���� ����
    int weight;                // ����ġ
    char url[256];       // URL �߰�
} Song;

// �� ����ü
typedef struct {
    Book books[MAX_ELEMENT];
    int book_count;
    Song songs[MAX_ELEMENT];
    int song_count;
} Heap;

// �帣 ����ġ ����ü
typedef struct {
    char book_genre[MAX_GENRE];
    char song_genre[MAX_GENRE];
    int weight;
} GenreWeight;

GenreWeight genre_weights[MAX_GENRE_WEIGHTS];
int genre_weight_count = 0;

// �� �ʱ�ȭ
Heap* create_heap() {
    Heap* h = (Heap*)malloc(sizeof(Heap));
    h->book_count = 0;
    h->song_count = 0;
    return h;
}

// ���ڿ� �յ� ���� ����
void trim(char* str) {
    int start = 0, end = strlen(str) - 1;

    while (start < strlen(str) && (str[start] == ' ' || str[start] == '\t')) {
        start++;
    }
    while (end >= 0 && (str[end] == ' ' || str[end] == '\t')) {
        end--;
    }

    str[end + 1] = '\0'; // ���� NULL ���� �߰�
    memmove(str, str + start, end - start + 2); // ���ڿ� �̵�
}

// ���ڿ� ���� '\r' ���� �Լ�
void remove_carriage_return(char* str) {
    size_t len = strlen(str);
    if (len > 0 && str[len - 1] == '\r') {
        str[len - 1] = '\0';
    }
}

// ������ �ε�
void load_data(Heap* heap, const char* filename, const char* type) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("������ �� �� �����ϴ�");
        exit(1);
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        remove_carriage_return(line);

        if (strcmp(type, "book") == 0) {
            Book book;
            if (sscanf(line, "%[^-] - %[^-] - %[^\n]", book.title, book.author, book.genre) == 3) {
                trim(book.title);
                trim(book.author);
                trim(book.genre);
                heap->books[heap->book_count++] = book;
            }
        }
        else if (strcmp(type, "song") == 0) {
            Song song;
            char genres[MAX_GENRE * 2];
            if (sscanf(line, "%[^-] - %[^-] - %[^-] - %[^\n]", song.title, song.artist, genres, song.url) == 4) {
                trim(song.title);
                trim(song.artist);
                trim(song.url);
                char* genre = strtok(genres, ",");
                int count = 0;
                while (genre && count < 2) {
                    trim(genre);
                    strcpy(song.genres[count++], genre);
                    genre = strtok(NULL, ",");
                }
                song.weight = 0;
                heap->songs[heap->song_count++] = song;
            }
            if (sscanf(line, "%[^-] - %[^-] - %[^-] - %[^\n]", song.title, song.artist, genres, song.url) != 4) {
                printf("���� �м� ����: %s\n", line);
                continue;
            }
        }
    }

    fclose(file);
}

void save_playlist_to_file(Song* songs, int count, const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        perror("������ �� �� �����ϴ�");
        exit(1);
    }

    for (int i = 0; i < count && i < 10; i++) {
        fprintf(file, "%s - %s - %s\n", songs[i].title, songs[i].artist, songs[i].url);  // URL ����
    }

    fclose(file);
}

void create_youtube_playlist() {
    // ���̽� ��ũ��Ʈ ȣ��
    system("python3 create_youtube_playlist.py");
}

// å ���� �Լ�
bool choose_book(Book* books, int book_count, const char* genre, char* selected_title) {
    printf("\n'%s' �帣�� å ���:\n", genre);
    int valid_books = 0;

    for (int i = 0; i < book_count; i++) {
        if (strcmp(books[i].genre, genre) == 0) {
            printf("- %s (����: %s)\n", books[i].title, books[i].author);
            valid_books++;
        }
    }

    if (valid_books == 0) {
        printf("�ش� �帣�� �´� å�� �����ϴ�.\n");
        return false;
    }

    // å ���� ����
    while (true) {
        printf("\nå ������ �Է��ϼ���: ");
        scanf(" %[^\n]", selected_title); // å ���� �Է� ����

        for (int i = 0; i < book_count; i++) {
            if (strcmp(books[i].title, selected_title) == 0 &&
                strcmp(books[i].genre, genre) == 0) {
                return true; // ���õ� å�� ��ȿ�� ���
            }
        }

        printf("'%s' �帣�� �ش��ϴ� å ������ �ƴմϴ�. �ٽ� �Է��ϼ���.\n", genre);
    }
}

// �帣 ����ġ �߰�
void add_genre_weight(const char* book_genre, const char* song_genre, int weight) {
    if (genre_weight_count < MAX_GENRE_WEIGHTS) {
        strcpy(genre_weights[genre_weight_count].book_genre, book_genre);
        strcpy(genre_weights[genre_weight_count].song_genre, song_genre);
        genre_weights[genre_weight_count++].weight = weight;
    }
}

void recommend_songs(Heap* heap, const char* book_genre) {
    Song recommendations[MAX_ELEMENT];
    int rec_count = 0;

    // �뷡 ����ġ ���
    for (int i = 0; i < heap->song_count; i++) {
        int total_weight = 0;

        for (int g = 0; g < 2; g++) {
            if (strlen(heap->songs[i].genres[g]) == 0) continue;

            bool matched = false;
            for (int j = 0; j < genre_weight_count; j++) {
                if (strcmp(book_genre, genre_weights[j].book_genre) == 0 &&
                    strcmp(heap->songs[i].genres[g], genre_weights[j].song_genre) == 0) {
                    total_weight += genre_weights[j].weight;
                    matched = true;
                    break;
                }
            }

            if (!matched) {
                total_weight += 1;
            }
        }

        heap->songs[i].weight = total_weight;

        if (total_weight > 0) {
            recommendations[rec_count++] = heap->songs[i];
        }
    }

    // ����ġ�� ���� (������ �߰�)
    srand(time(NULL));
    for (int i = 0; i < rec_count - 1; i++) {
        for (int j = i + 1; j < rec_count; j++) {
            if (recommendations[i].weight < recommendations[j].weight ||
                (recommendations[i].weight == recommendations[j].weight && rand() % 2 == 0)) {
                Song temp = recommendations[i];
                recommendations[i] = recommendations[j];
                recommendations[j] = temp;
            }
        }
    }

    // ��õ ���
    if (rec_count == 0) {
        printf("\n��õ�� ������ �����ϴ�.\n");
        return;
    }

    printf("\n��õ ���� ����Ʈ:\n");
    for (int i = 0; i < (rec_count > 10 ? 10 : rec_count); i++) {
        char genre_display[MAX_GENRE * 2] = "";
        if (strlen(recommendations[i].genres[1]) > 0 &&
            strcmp(recommendations[i].genres[0], recommendations[i].genres[1]) != 0) {
            snprintf(genre_display, sizeof(genre_display), "%s, %s",
                recommendations[i].genres[0], recommendations[i].genres[1]);
        }
        else {
            snprintf(genre_display, sizeof(genre_display), "%s", recommendations[i].genres[0]);
        }

        printf("- %s (��Ƽ��Ʈ: %s / �帣: %s / ����ġ �հ�: %d)\n",
            recommendations[i].title,
            recommendations[i].artist,
            genre_display,
            recommendations[i].weight);
    }


    // ��õ�� �뷡�� playlist.txt ���Ͽ� ����
    save_playlist_to_file(recommendations, rec_count, "playlist.txt");

    printf("\n��õ�� �뷡�� playlist.txt�� ����Ǿ����ϴ�.\n");

    // �� �� Python ��ũ��Ʈ�� ȣ��
    create_youtube_playlist();
}

// �帣 ���� �߰� ����ȭ
void initialize_genre_weights() {
    const char* mappings[][3] = {
        {"��Ÿ��", "Ŭ����(����)", "4"},
        {"��Ÿ��", "��ȭOST", "5"},
        {"��Ÿ��", "�ε�", "3"},
        {"�̽��͸�", "�ں��Ʈ", "5"},
        {"�̽��͸�", "Ŭ����(��ο�)", "4"},
        {"�̽��͸�", "����", "2"},
        {"�θǽ�", "����ƽ", "4"},
        {"�θǽ�", "�߶��", "5"},
        {"�θǽ�", "�ε�", "3"},
        {"SF", "�ں��Ʈ", "4"},
        {"SF", "Ŭ����(��ο�)", "5"},
        {"SF", "����", "2"},
        {"�ڱⰳ��", "Ŭ����(����)", "5" },
        {"�ڱⰳ��", "�ε�", "3" },
        {"�ڱⰳ��", "����", "2" },
        {"����", "Ŭ����(��ο�)", "4"},
        {"����", "������", "5"},
        {"����", "����", "2"}
    };

    for (int i = 0; i < sizeof(mappings) / sizeof(mappings[0]); i++) {
        add_genre_weight(mappings[i][0], mappings[i][1], atoi(mappings[i][2]));
    }
}

void debug_songs(Heap* heap) {
    printf("�ε��� �뷡 ����Ʈ:\n");
    for (int i = 0; i < heap->song_count; i++) {
        printf("- ����: %s, ��Ƽ��Ʈ: %s, �帣: %s, %s\n",
            heap->songs[i].title,
            heap->songs[i].artist,
            heap->songs[i].genres[0],
            strlen(heap->songs[i].genres[1]) > 0 ? heap->songs[i].genres[1] : "(����)");
    }
}


// å �帣���� ����ϴ� �Լ�
void print_available_genres(Heap* heap) {
    char unique_genres[MAX_ELEMENT][MAX_GENRE];
    int unique_count = 0;

    printf("\n��� ������ å �帣 ���:\n");

    for (int i = 0; i < heap->book_count; i++) {
        bool is_unique = true;

        // ���� å�� �帣�� �̹� ��Ͽ� �ִ��� Ȯ��
        for (int j = 0; j < unique_count; j++) {
            if (strcmp(heap->books[i].genre, unique_genres[j]) == 0) {
                is_unique = false;
                break;
            }
        }

        // ������ �帣��� �߰�
        if (is_unique) {
            strcpy(unique_genres[unique_count++], heap->books[i].genre);
            printf("- %s\n", heap->books[i].genre);
        }
    }

    if (unique_count == 0) {
        printf("��� ������ �帣�� �����ϴ�.\n");
    }
}

// ���� �Լ�
int main() {
    Heap* heap = create_heap();

    // ������ �ε�
    load_data(heap, "books.txt", "book");
    load_data(heap, "songs.txt", "song");

    // �帣 ����ġ �ʱ�ȭ
    initialize_genre_weights();

    // ��� ������ å �帣 ���
    print_available_genres(heap);

    char genre[MAX_GENRE];
    char selected_title[MAX_TITLE];

    // å �帣 �Է� �ޱ�
    while (true) {
        printf("å �帣�� �Է��ϼ��� (�����Ϸ��� '����' �Է�): ");
        if (scanf(" %[^\n]", genre) != 1) {
            printf("�Է� ����! å �帣�� ��Ȯ�� �Է��ϼ���.\n");
            continue;  // �߸��� �Է��̸� �ٽ� �Է� �ޱ�
        }

        // '����' �Է� �� ���α׷� ����
        if (strcmp(genre, "����") == 0) {
            printf("���α׷��� �����մϴ�.\n");
            break;
        }

        // �帣 ��Ͽ� �ִ��� Ȯ��
        bool genre_found = false;
        for (int i = 0; i < heap->book_count; i++) {
            if (strcmp(heap->books[i].genre, genre) == 0) {
                genre_found = true;
                break;
            }
        }

        if (genre_found) {
            break;  // �帣�� ��Ͽ� ������ ����
        }
        else {
            printf("�帣�� ��Ͽ� �����ϴ�. �ٽ� �Է��ϼ���.\n");
        }
    }

    if (choose_book(heap->books, heap->book_count, genre, selected_title)) {
        printf("\n������ å ����: %s\n", selected_title);
        recommend_songs(heap, genre);
    }
    else {
        printf("å ������ �Ϸ���� �ʾҽ��ϴ�.\n");
    }

    free(heap);
    return 0;
}