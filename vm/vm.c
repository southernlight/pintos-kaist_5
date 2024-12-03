/* vm.c: Generic interface for virtual memory objects. */

#include "vm/vm.h"
#include "threads/malloc.h"
#include "vm/inspect.h"

/* project3-Implement Supplemental Page Table */
#include "lib/kernel/hash.h"

#include "threads/vaddr.h"

/* project3-Frame Management */
static struct list frame_table;

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void vm_init(void) {
  vm_anon_init();
  vm_file_init();
#ifdef EFILESYS /* For project 4 */
  pagecache_init();
#endif
  register_inspect_intr();
  /* DO NOT MODIFY UPPER LINES. */
  /* TODO: Your code goes here. */
  /* project3-Frame Management */
  list_init(&frame_table);
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type page_get_type(struct page *page) {
  int ty = VM_TYPE(page->operations->type);
  switch (ty) {
  case VM_UNINIT:
    return VM_TYPE(page->uninit.type);
  default:
    return ty;
  }
}

/* Helpers */
static struct frame *vm_get_victim(void);
static bool vm_do_claim_page(struct page *page);
static struct frame *vm_evict_frame(void);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
bool vm_alloc_page_with_initializer(enum vm_type type, void *upage,
                                    bool writable, vm_initializer *init,
                                    void *aux) {
  ASSERT(VM_TYPE(type) != VM_UNINIT)
  struct supplemental_page_table *spt = &thread_current()->spt;
  // printf("I'm %p\n", spt);

  // 체크 코드
  // printf("%p\n", upage);
  // struct page *existing_page = spt_find_page(spt, upage);
  // if (existing_page != NULL) {
  //   printf("Page already exists: %p\n", upage);
  //   return false;
  // }

  /* Check wheter the upage is already occupied or not. */
  if (spt_find_page(spt, upage) == NULL) {
    /* TODO: Create the page, fetch the initialier according to the VM type,
     * TODO: and then create "uninit" page struct by calling uninit_new. You
     * TODO: should modify the field after calling the uninit_new. */
    /* project3-Lazy Loading for Executable */
    struct page *p = malloc(sizeof(struct page));
    if (p == NULL)
      return false;
    if (type == VM_ANON) {
      uninit_new(p, upage, init, type, aux, anon_initializer);
    } else {
      uninit_new(p, upage, init, type, aux, file_backed_initializer);
    }
    p->writable = writable;
    /* TODO: Insert the page into the spt. */
    if (!spt_insert_page(spt, p)) {
      free(p);
      return false;
    }
    return true;
  }
  // printf("helloworld!\n");
  return false;
err:
  return false;
}

/* Find VA from spt and return page. On error, return NULL. */
struct page *spt_find_page(struct supplemental_page_table *spt UNUSED,
                           void *va UNUSED) {
  struct page *page = NULL;
  /* TODO: Fill this function. */

  /* project3-Implement Supplemental Page Table */
  page = (struct page *)malloc(sizeof(struct page));
  page->va = pg_round_down(va);
  struct hash_elem *e;
  // 블로그랑 다름
  // page->va = va;
  e = hash_find(&spt->spt_hash, &page->hash_elem);

  if (e != NULL)
    return hash_entry(e, struct page, hash_elem);

  free(page);
  return NULL;
}

/* Insert PAGE into spt with validation. */
bool spt_insert_page(struct supplemental_page_table *spt UNUSED,
                     struct page *page UNUSED) {
  // bool succ = false;
  /* TODO: Fill this function. */

  /* project3-Implement Supplemental Page Table */
  // 블로그랑 다름
  // if (hash_insert(&spt->spt_hash, &page->hash_elem) == NULL)
  //   succ = true;

  // return succ;
  /* TODO: Fill this function. */
  return hash_insert(&spt->spt_hash, &page->hash_elem) == NULL ? true : false;
}

void spt_remove_page(struct supplemental_page_table *spt, struct page *page) {
  vm_dealloc_page(page);
  return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *vm_get_victim(void) {
  struct frame *victim = NULL;
  /* TODO: The policy for eviction is up to you. */

  return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *vm_evict_frame(void) {
  struct frame *victim UNUSED = vm_get_victim();
  /* TODO: swap out the victim and return the evicted frame. */

  return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
/* project3-Frame Management */
static struct frame *vm_get_frame(void) {
  /* TODO: Fill this function. */
  struct frame *frame = malloc(sizeof(struct frame));
  frame->page = NULL;

  // error 해결
  ASSERT(frame != NULL);
  ASSERT(frame->page == NULL);
  uint8_t *kpage = palloc_get_page(PAL_USER);
  if (kpage == NULL) {
    PANIC("todo");
  }
  frame->kva = kpage;
  list_push_back(&frame_table, &frame->frame_elem);
  return frame;
}

/* Growing the stack. */
static void vm_stack_growth(void *addr UNUSED) {}

/* Handle the fault on write_protected page */
static bool vm_handle_wp(struct page *page UNUSED) {}

/* Return true on success */
bool vm_try_handle_fault(struct intr_frame *f UNUSED, void *addr UNUSED,
                         bool user UNUSED, bool write UNUSED,
                         bool not_present UNUSED) {
  struct supplemental_page_table *spt UNUSED = &thread_current()->spt;
  struct page *page = spt_find_page(spt, addr);

  /* TODO: Validate the fault */
  /* TODO: Your code goes here */

  // bool not_present; /* True: not-present page, false: writing r/o page. */
  // bool write;       /* True: access was write, false: access was read. */
  // bool user;        /* True: access by user, false: access by kernel. */
  // void *fault_addr; /* Fault address. */

  /* Case 1: 유효하지 않은 주소에 대한 접근인 경우 */
  if (is_kernel_vaddr(addr) || addr == NULL) {
    exit(-1);
  }

  /*
  Case 2 페이지가 존재하지 않는 경우
  Case 3 또는 페이지는 존재하는데 읽기 전용 페이지에 쓰기 시도 경우
  */
  if (not_present) {
    page = spt_find_page(spt, addr);
    if (page == NULL) {
      exit(-1);
      return false;
    }

    if (write && page->writable == false)
      return false;

    return vm_do_claim_page(page);
  }

  /* Case 3: 페이지가 존재하고, 읽기 전용 페이지에 쓰기 시도한 경우 */
  if (write && page != NULL && page->writable == false) {
    return false; // 읽기 전용 페이지에 대한 쓰기 접근
  }

  return false;

  // return vm_do_claim_page(page);
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void vm_dealloc_page(struct page *page) {
  destroy(page);
  free(page);
}

/* Claim the page that allocate on VA. */
bool vm_claim_page(void *va UNUSED) {
  /* TODO: Fill this function */
  /* project3-Frame Management */
  struct page *page = malloc(sizeof(struct page));
  struct thread *cur = thread_current();
  struct hash_elem *e;
  if (page == NULL)
    return false;
  page->va = va;
  e = hash_find(&(cur->spt.spt_hash), &page->hash_elem);
  if (e == NULL)
    return false;
  page = hash_entry(e, struct page, hash_elem);
  return vm_do_claim_page(page);
}
/* Claim the PAGE and set up the mmu. */
static bool vm_do_claim_page(struct page *page) {
  /* project3-Frame Management */
  struct frame *frame = vm_get_frame();
  /* Set links */
  frame->page = page;
  page->frame = frame;
  /* TODO: Insert page table entry to map page's VA to frame's PA. */
  // bool pml4_set_page (uint64_t *pml4, void *upage, void *kpage, bool rw);
  // upage 에는 page -> va 가 들어가면 되고, kpage 에는 frame->kva 가 들어가면
  // 된다.
  /* project3-Implement Frame Management */
  //   if (page->operations->type == NULL)
  //     return false;
  //   if (page->operations->type == VM_FILE)
  //     pml4_set_page(thread_current()->pml4, page->va, frame->kva, true);
  //   else
  //     pml4_set_page(thread_current()->pml4, page->va, frame->kva, false);
  if (!pml4_set_page(thread_current()->pml4, page->va, frame->kva,
                     page->writable))
    return false;
  return swap_in(page, frame->kva);
}

/* project3-Implement Supplemental Page Table */
unsigned page_hash(const struct hash_elem *e, void *aux) {
  struct page *p = hash_entry(e, struct page, hash_elem);
  // 키를 해시 값으로 변환
  // return hash_int(p->va);
  return hash_bytes(&p->va, sizeof p->va);
}

/* project3-Implement Supplemental Page Table */
/* 페이지를 비교하여, 가상 주소 순으로 정렬 */
bool page_less(const struct hash_elem *a, const struct hash_elem *b,
               void *aux) {
  struct page *page_a = hash_entry(a, struct page, hash_elem);
  struct page *page_b = hash_entry(b, struct page, hash_elem);

  // 가상 주소를 비교하여 페이지들 간의 순서를 결정
  return page_a->va < page_b->va;
}
/* Initialize new supplemental page table */
void supplemental_page_table_init(struct supplemental_page_table *spt UNUSED) {
  /* project3-Implement Supplemental Page Table */
  hash_init(spt, page_hash, page_less, NULL);
}

/* Copy supplemental page table from src to dst */
bool supplemental_page_table_copy(struct supplemental_page_table *dst UNUSED,
                                  struct supplemental_page_table *src UNUSED) {
  struct hash_iterator i;
  hash_first(&i, &src->spt_hash);
  while (hash_next(&i)) {
    // src_page 정보
    struct page *src_page = hash_entry(hash_cur(&i), struct page, hash_elem);
    enum vm_type type = src_page->operations->type;
    void *upage = src_page->va;
    bool writable = src_page->writable;

    /* 1) type이 uninit이면 */
    if (type == VM_UNINIT) { // uninit page 생성 & 초기화
      vm_initializer *init = src_page->uninit.init;
      void *aux = src_page->uninit.aux;
      vm_alloc_page_with_initializer(VM_ANON, upage, writable, init, aux);
      continue;
    }

    /* 2) type이 uninit이 아니면 */
    if (!vm_alloc_page(type, upage, writable)) // uninit page 생성 & 초기화
      // init이랑 aux는 Lazy Loading에 필요함
      // 지금 만드는 페이지는 기다리지 않고 바로 내용을 넣어줄 것이므로 필요
      // 없음
      return false;

    // vm_claim_page으로 요청해서 매핑 & 페이지 타입에 맞게 초기화
    if (!vm_claim_page(upage))
      return false;

    // 매핑된 프레임에 내용 로딩
    struct page *dst_page = spt_find_page(dst, upage);
    memcpy(dst_page->frame->kva, src_page->frame->kva, PGSIZE);
  }
  return true;
}

void hash_page_destroy(struct hash_elem *e, void *aux) {
  struct page *page = hash_entry(e, struct page, hash_elem);
  destroy(page);
  free(page);
}

/* Free the resource hold by the supplemental page table */
void supplemental_page_table_kill(struct supplemental_page_table *spt UNUSED) {
  /* TODO: Destroy all the supplemental_page_table hold by thread and
   * TODO: writeback all the modified contents to the storage. */
  hash_clear(&spt->spt_hash,
             hash_page_destroy); // 해시 테이블의 모든 요소를 제거
}
