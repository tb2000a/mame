// license:BSD-3-Clause
// copyright-holders:Ernesto Corvi
#ifndef MAME_SOUND_2610INTF_H
#define MAME_SOUND_2610INTF_H

#pragma once

#include "ay8910.h"


struct ssg_callbacks;


#define MCFG_YM2610_IRQ_HANDLER(cb) \
		devcb = &downcast<ym2610_device &>(*device).set_irq_handler((DEVCB_##cb));

class ym2610_device : public ay8910_device
{
public:
	ym2610_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// configuration helpers
	template <class Object> devcb_base &set_irq_handler(Object &&cb) { return m_irq_handler.set_callback(std::forward<Object>(cb)); }

	DECLARE_READ8_MEMBER( read );
	DECLARE_WRITE8_MEMBER( write );

	// update request from fm.cpp
	static void update_request(device_t *param) { downcast<ym2610_device *>(param)->update_request(); }

	static char const *const YM2610_TAG;
	static char const *const YM2610_DELTAT_TAG;

protected:
	ym2610_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock);

	// device-level overrides
	virtual void device_start() override;
	virtual void device_post_load() override;
	virtual void device_stop() override;
	virtual void device_reset() override;

	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr) override;

	virtual void stream_generate(sound_stream &stream, stream_sample_t **inputs, stream_sample_t **outputs, int samples);

	void *          m_chip;

private:
	void irq_handler(int irq);
	void timer_handler(int c, int count, int clock);
	void update_request() { m_stream->update(); }

	static void static_irq_handler(device_t *param, int irq) { downcast<ym2610_device *>(param)->irq_handler(irq); }
	static void static_timer_handler(device_t *param, int c, int count, int clock) { downcast<ym2610_device *>(param)->timer_handler(c, count, clock); }

	// internal state
	sound_stream *  m_stream;
	emu_timer *     m_timer[2];
	devcb_write_line m_irq_handler;
	required_memory_region m_region;

	static const ssg_callbacks psgintf;
};


class ym2610b_device : public ym2610_device
{
public:
	ym2610b_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

protected:
	virtual void stream_generate(sound_stream &stream, stream_sample_t **inputs, stream_sample_t **outputs, int samples) override;
};


DECLARE_DEVICE_TYPE(YM2610,  ym2610_device)
DECLARE_DEVICE_TYPE(YM2610B, ym2610b_device)

#endif // MAME_SOUND_2610INTF_H
