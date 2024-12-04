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


// 책 구조체
typedef struct {
    char title[MAX_TITLE];
    char author[MAX_TITLE];
    char genre[MAX_GENRE];
} Book;

// 노래 구조체
typedef struct {
    char title[MAX_TITLE];
    char artist[MAX_TITLE];
    char genres[2][MAX_GENRE]; // 장르 2개 저장 가능
    int weight;                // 가중치
    char url[256];       // URL 추가
} Song;

// 힙 구조체
typedef struct {
    Book books[MAX_ELEMENT];
    int book_count;
    Song songs[MAX_ELEMENT];
    int song_count;
} Heap;

// 장르 가중치 구조체
typedef struct {
    char book_genre[MAX_GENRE];
    char song_genre[MAX_GENRE];
    int weight;
} GenreWeight;

GenreWeight genre_weights[MAX_GENRE_WEIGHTS];
int genre_weight_count = 0;

// 힙 초기화
Heap* create_heap() {
    Heap* h = (Heap*)malloc(sizeof(Heap));
    h->book_count = 0;
    h->song_count = 0;
    return h;
}

// 문자열 앞뒤 공백 제거
void trim(char* str) {
    int start = 0, end = strlen(str) - 1;

    while (start < strlen(str) && (str[start] == ' ' || str[start] == '\t')) {
        start++;
    }
    while (end >= 0 && (str[end] == ' ' || str[end] == '\t')) {
        end--;
    }

    str[end + 1] = '\0'; // 끝에 NULL 문자 추가
    memmove(str, str + start, end - start + 2); // 문자열 이동
}

// 문자열 끝의 '\r' 제거 함수
void remove_carriage_return(char* str) {
    size_t len = strlen(str);
    if (len > 0 && str[len - 1] == '\r') {
        str[len - 1] = '\0';
    }
}

// 데이터 로드
void load_data(Heap* heap, const char* filename, const char* type) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("파일을 열 수 없습니다");
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
                printf("라인 분석 실패: %s\n", line);
                continue;
            }
        }
    }

    fclose(file);
}

void save_playlist_to_file(Song* songs, int count, const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        perror("파일을 열 수 없습니다");
        exit(1);
    }

    for (int i = 0; i < count && i < 10; i++) {
        fprintf(file, "%s - %s - %s\n", songs[i].title, songs[i].artist, songs[i].url);  // URL 포함
    }

    fclose(file);
}

// 책 선택 함수
bool choose_book(Book* books, int book_count, const char* genre, char* selected_title) {
    printf("\n'%s' 장르의 책 목록:\n", genre);
    int valid_books = 0;

    for (int i = 0; i < book_count; i++) {
        if (strcmp(books[i].genre, genre) == 0) {
            printf("- %s (저자: %s)\n", books[i].title, books[i].author);
            valid_books++;
        }
    }

    if (valid_books == 0) {
        printf("해당 장르에 맞는 책이 없습니다.\n");
        return false;
    }

    // 책 제목 선택
    while (true) {
        printf("\n책 제목을 입력하세요: ");
        scanf(" %[^\n]", selected_title); // 책 제목 입력 받음

        for (int i = 0; i < book_count; i++) {
            if (strcmp(books[i].title, selected_title) == 0 &&
                strcmp(books[i].genre, genre) == 0) {
                return true; // 선택된 책이 유효한 경우
            }
        }

        printf("'%s' 장르에 해당하는 책 제목이 아닙니다. 다시 입력하세요.\n", genre);
    }
}

// 장르 가중치 추가
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

    // 노래 가중치 계산
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

    // 가중치로 정렬 (랜덤성 추가)
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

    // 추천 출력
    if (rec_count == 0) {
        printf("\n추천할 음악이 없습니다.\n");
        return;
    }

    printf("\n추천 음악 리스트:\n");
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

        printf("- %s (아티스트: %s / 장르: %s / 가중치 합계: %d)\n",
            recommendations[i].title,
            recommendations[i].artist,
            genre_display,
            recommendations[i].weight);
    }


    // 추천된 노래를 playlist.txt 파일에 저장
    save_playlist_to_file(recommendations, rec_count, "playlist.txt");

    printf("\n추천된 노래가 playlist.txt에 저장되었습니다.\n");

    // 그 후 Python 스크립트를 호출
    create_youtube_playlist();
}

// 장르 매핑 추가 최적화
void initialize_genre_weights() {
    const char* mappings[][3] = {
        {"판타지", "클래식(밝음)", "4"},
        {"판타지", "영화OST", "5"},
        {"판타지", "인디", "3"},
        {"미스터리", "앰비언트", "5"},
        {"미스터리", "클래식(어두움)", "4"},
        {"미스터리", "재즈", "2"},
        {"로맨스", "어쿠스틱", "4"},
        {"로맨스", "발라드", "5"},
        {"로맨스", "인디", "3"},
        {"SF", "앰비언트", "4"},
        {"SF", "클래식(어두움)", "5"},
        {"SF", "재즈", "2"},
        {"자기개발", "클래식(밝음)", "5" },
        {"자기개발", "인디", "3" },
        {"자기개발", "재즈", "2" },
        {"역사", "클래식(어두움)", "4"},
        {"역사", "로파이", "5"},
        {"역사", "재즈", "2"}
    };

    for (int i = 0; i < sizeof(mappings) / sizeof(mappings[0]); i++) {
        add_genre_weight(mappings[i][0], mappings[i][1], atoi(mappings[i][2]));
    }
}

void debug_songs(Heap* heap) {
    printf("로딩된 노래 리스트:\n");
    for (int i = 0; i < heap->song_count; i++) {
        printf("- 제목: %s, 아티스트: %s, 장르: %s, %s\n",
            heap->songs[i].title,
            heap->songs[i].artist,
            heap->songs[i].genres[0],
            strlen(heap->songs[i].genres[1]) > 0 ? heap->songs[i].genres[1] : "(없음)");
    }
}


// 책 장르들을 출력하는 함수
void print_available_genres(Heap* heap) {
    char unique_genres[MAX_ELEMENT][MAX_GENRE];
    int unique_count = 0;

    printf("\n사용 가능한 책 장르 목록:\n");

    for (int i = 0; i < heap->book_count; i++) {
        bool is_unique = true;

        // 현재 책의 장르가 이미 목록에 있는지 확인
        for (int j = 0; j < unique_count; j++) {
            if (strcmp(heap->books[i].genre, unique_genres[j]) == 0) {
                is_unique = false;
                break;
            }
        }

        // 고유한 장르라면 추가
        if (is_unique) {
            strcpy(unique_genres[unique_count++], heap->books[i].genre);
            printf("- %s\n", heap->books[i].genre);
        }
    }

    if (unique_count == 0) {
        printf("사용 가능한 장르가 없습니다.\n");
    }
}

// 메인 함수
int main() {
    Heap* heap = create_heap();

    // 데이터 로드
    load_data(heap, "books.txt", "book");
    load_data(heap, "songs.txt", "song");

    // 장르 가중치 초기화
    initialize_genre_weights();

    // 사용 가능한 책 장르 출력
    print_available_genres(heap);

    char genre[MAX_GENRE];
    char selected_title[MAX_TITLE];

    // 책 장르 입력 받기
    while (true) {
        printf("책 장르를 입력하세요 (종료하려면 '종료' 입력): ");
        if (scanf(" %[^\n]", genre) != 1) {
            printf("입력 오류! 책 장르를 정확히 입력하세요.\n");
            continue;  // 잘못된 입력이면 다시 입력 받기
        }

        // '종료' 입력 시 프로그램 종료
        if (strcmp(genre, "종료") == 0) {
            printf("프로그램을 종료합니다.\n");
            break;
        }

        // 장르 목록에 있는지 확인
        bool genre_found = false;
        for (int i = 0; i < heap->book_count; i++) {
            if (strcmp(heap->books[i].genre, genre) == 0) {
                genre_found = true;
                break;
            }
        }

        if (genre_found) {
            break;  // 장르가 목록에 있으면 종료
        }
        else {
            printf("장르가 목록에 없습니다. 다시 입력하세요.\n");
        }
    }

    if (choose_book(heap->books, heap->book_count, genre, selected_title)) {
        printf("\n선택한 책 제목: %s\n", selected_title);
        recommend_songs(heap, genre);
    }
    else {
        printf("책 선택이 완료되지 않았습니다.\n");
    }

    free(heap);
    return 0;
}
