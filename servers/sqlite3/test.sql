CREATE TABLE users (
  id INTEGER PRIMARY KEY,
  name TEXT NOT NULL,
  email TEXT NOT NULL UNIQUE
);

INSERT INTO users (name, email) VALUES ('John Smith', 'john@example.com');
INSERT INTO users (name, email) VALUES ('Lorem Ipsum', 'lorem@example.com');
