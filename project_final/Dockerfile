FROM node:20-bookworm

WORKDIR /app

RUN apt-get update && apt-get install -y g++ \
    && rm -rf /var/lib/apt/lists/*

COPY package*.json ./
RUN npm install --omit=dev

COPY . .

RUN g++ cpp/main.cpp -o cpp/md2html.exe -std=c++17

EXPOSE 3000

CMD ["npm", "start"]
