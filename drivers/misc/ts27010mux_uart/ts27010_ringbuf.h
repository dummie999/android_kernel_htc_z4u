#ifndef __TS27010_RINGBUF_H__
#define __TS27010_RINGBUF_H__

struct ts27010_ringbuf {
	int len;
	int head;
	int tail;
	u8 buf[];
};

static inline struct ts27010_ringbuf *ts27010_ringbuf_alloc(int len)
{
	struct ts27010_ringbuf *rbuf;

	rbuf = kmalloc(sizeof(*rbuf) + len, GFP_KERNEL);
	if (rbuf == NULL) {
		mux_print(MSG_WARNING, "request memory failed\n");
		return NULL;
	}

	rbuf->len = len;
	rbuf->head = 0;
	rbuf->tail = 0;

	return rbuf;
}

static inline void ts27010_ringbuf_free(struct ts27010_ringbuf *rbuf)
{
	kfree(rbuf);
}

static inline int ts27010_ringbuf_level(struct ts27010_ringbuf *rbuf)
{
	int level = rbuf->head - rbuf->tail;

	if (level < 0)
		level = rbuf->len + level;

	return level;
}

static inline int ts27010_ringbuf_room(struct ts27010_ringbuf *rbuf)
{
	return rbuf->len - ts27010_ringbuf_level(rbuf) - 1;
}

static inline u8 ts27010_ringbuf_peek(struct ts27010_ringbuf *rbuf, int i)
{
	return rbuf->buf[(rbuf->tail + i) % rbuf->len];
}

static inline int ts27010_ringbuf_read(
	struct ts27010_ringbuf *rbuf, int idx, u8* buf, int len)
{
#if 1
	int buf_level = ts27010_ringbuf_level(rbuf);
	int i;
	FUNC_ENTER();

	if (!buf)
		return -ENOMEM;

	if (idx > buf_level || idx < 0) {
		mux_print(MSG_ERROR,
			"invalid argument idx: %d while level = %d\n",
			idx, buf_level);
		return -EINVAL;
	}

	if ((len + idx) > buf_level)
		mux_print(MSG_WARNING,
			"only can read part data, len: %d, level: %d\n",
			len + idx, buf_level);

	for (i = 0; i < len; i++)
		buf[i] = ts27010_ringbuf_peek(rbuf, i + idx);

	FUNC_EXIT();
	return len;
#else
	int buf_level = ts27010_ringbuf_level(rbuf);
	int count;
	FUNC_ENTER();

	if (!buf)
		return -ENOMEM;

	if (idx > buf_level || idx < 0) {
		mux_print(MSG_ERROR,
			"invalid argument idx: %d while level = %d\n",
			idx, buf_level);
		return -EINVAL;
	}

	if ((len + idx) > buf_level)
		mux_print(MSG_WARNING,
			"only can read part data, len: %d, level: %d\n",
			len + idx, buf_level);

	count = (len <= (buf_level - idx)) ? len : (buf_level - idx);
	if (count == 0) {
		mux_print(MSG_ERROR, "rbuf empty: head %d, tail %d, len %d\n",
			rbuf->head, rbuf->tail, rbuf->len);
	} else if (rbuf->head > rbuf->tail) {
		memcpy(buf, rbuf->buf + rbuf->tail + idx, count);
	} else {
		if (rbuf->tail + idx + count >= rbuf->len) {
			int rest = rbuf->len - rbuf->tail - idx;
			memcpy(buf, rbuf->buf + rbuf->tail + idx, rest);
			memcpy(buf + rest, rbuf->buf, count - rest);
		} else {
			memcpy(buf, rbuf->buf + rbuf->tail + idx, count);
		}
	}
	return count;
#endif
}

static inline int ts27010_ringbuf_consume(struct ts27010_ringbuf *rbuf,
					  int count)
{
	count = min(count, ts27010_ringbuf_level(rbuf));

	rbuf->tail = (rbuf->tail + count) % rbuf->len;

	return count;
}

static inline int ts27010_ringbuf_push(struct ts27010_ringbuf *rbuf, u8 datum)
{
	if (ts27010_ringbuf_room(rbuf) == 0)
		return 0;

	rbuf->buf[rbuf->head] = datum;
	rbuf->head = (rbuf->head + 1) % rbuf->len;

	return 1;
}

static inline int ts27010_ringbuf_write(struct ts27010_ringbuf *rbuf,
					const u8 *data, int len)
{
	int count = 0;
#if 1
	int buf_room = ts27010_ringbuf_room(rbuf);
	FUNC_ENTER();
	
	mux_print(MSG_DEBUG, "Data length: %d byte \n", len);
	

	count = (len <= buf_room) ? len : buf_room;
	if (count == 0) {
		mux_print(MSG_ERROR, "rbuf full: head %d, tail %d, len %d\n",
			rbuf->head, rbuf->tail, rbuf->len);
	} else if (rbuf->head >= rbuf->tail) {
		if ((count + rbuf->head) > rbuf->len) {
			int rest = rbuf->len - rbuf->head;
			memcpy(rbuf->buf + rbuf->head, data, rest);
			memcpy(rbuf->buf, data + rest, count - rest);
		} else {
			memcpy(rbuf->buf + rbuf->head, data, count);
		}
	} else {
		memcpy(rbuf->buf + rbuf->head, data, count);
	}
	rbuf->head = (rbuf->head + count) % rbuf->len;
#else
	int i;

	for (i = 0; i < len; i++)
		count += ts27010_ringbuf_push(rbuf, data[i]);
#endif

	FUNC_EXIT();
	return count;
}
#if 0
static inline int ts27010_mux_log_ringbuf_read(
	struct ts27010_ringbuf *rbuf, int idx, u8* buf, int len)
{
	int buf_level = ts27010_ringbuf_level(rbuf);
	int i;
	FUNC_ENTER();

	if (!buf)
		return -ENOMEM;

	if (idx > buf_level || idx < 0) {
		mux_print(MSG_ERROR,
			"invalid argument idx: %d while level = %d\n",
			idx, buf_level);
		return -EINVAL;
	}

	if ((len + idx) > buf_level)
		mux_print(MSG_WARNING,
			"only can read part data, len: %d, level: %d\n",
			len + idx, buf_level);

	for (i = 0; i < len; i++)
		buf[i] = ts27010_ringbuf_peek(rbuf, i + idx);

        rbuf->tail = (rbuf->tail + len) % rbuf->len;
	FUNC_EXIT();
	return len;
}

static inline int ts27010_mux_log_write(struct ts27010_ringbuf *rbuf,
					const u8 *data, int len)
{
	int count = 0;
	int rest = 0;
	int lost = 0;
	FUNC_ENTER();
	
	mux_print(MSG_DEBUG, "Data length: %d byte \n", len);
	
	count = len;
	if (rbuf->head >= rbuf->tail) {
		if ((count + rbuf->head) > rbuf->len) {
			rest = rbuf->len - rbuf->head;
			memcpy(rbuf->buf + rbuf->head, data, rest);
			memcpy(rbuf->buf, data + rest, count - rest);
			if (((count + rbuf->head) % rbuf->len) >= rbuf->tail) {
				lost += ((count + rbuf->head) % rbuf->len) - rbuf->tail + 1;
				rbuf->tail = (count + rbuf->head + 1) % rbuf->len;
			}
		} else {
			memcpy(rbuf->buf + rbuf->head, data, count);
		}
	} else {
		if ((count + rbuf->head) >= rbuf->tail) {
			if ((count + rbuf->head) > rbuf->len) {
				rest = rbuf->len - rbuf->head;
				memcpy(rbuf->buf + rbuf->head, data, rest);
				memcpy(rbuf->buf, data + rest, count - rest);
			} else {
				memcpy(rbuf->buf + rbuf->head, data, count);
			}
			lost += count - (rbuf->tail - rbuf->head) + 1;
			rbuf->tail = (count + rbuf->head + 1) % rbuf->len;
		} else {
			memcpy(rbuf->buf + rbuf->head, data, count);
		}
	}

	rbuf->head = (rbuf->head + count) % rbuf->len;

	FUNC_EXIT();
	return lost;
}
#endif
#endif 
