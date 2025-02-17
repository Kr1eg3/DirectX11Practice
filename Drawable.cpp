#include "Drawable.h"
#include "GraphicsThrowMacros.h"
#include "IndexBuffer.h"
#include <cassert>


void Drawable::Draw(Graphics& gfx) const noexcept(!IS_DEBUG) {
	for (auto& b : binds) {
		b->Bind(gfx);
	}

	auto& staticBinds = GetStaticBinds();
	for (auto& b : staticBinds) {
		b->Bind(gfx);
	}

	auto idxCount = pIndexBuffer->GetCount();
	gfx.DrawIndexed(idxCount);
}

void Drawable::AddBind(std::unique_ptr<Bindable> bind) noexcept(!IS_DEBUG) {
	assert("*Must* use AddIndexBuffer to bind index buffer" && typeid(*bind) != typeid(IndexBuffer));
	binds.push_back(std::move(bind));
}

void Drawable::AddIndexBuffer(std::unique_ptr<IndexBuffer> ibuf) noexcept(!IS_DEBUG) {
	assert("Attempting to add index buffer a second time" && pIndexBuffer == nullptr);
	pIndexBuffer = ibuf.get();
	binds.push_back(std::move(ibuf));
}